#pragma once
//
// A small persistent worker pool used to parallelize the hot reduction loop in
// generation (the gcd combination-selection scan). It is owned by a Task and is only
// ever driven from that Task's single generation thread - workers never call back
// into ParallelFor - so the public API needs no locking against concurrent
// submission.
//
// Determinism note: parallelFor makes NO ordering guarantee. It only promises that
// fn(i) is invoked exactly once for every i in [begin, end). Callers must therefore
// write results into pre-sized, index-addressed storage and perform any ordered
// work (selection, RNG draws) serially afterwards. This is what lets the engine be
// both parallel and bit-for-bit reproducible.
//

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

namespace pictcore
{

class ThreadPool
{
public:
    explicit ThreadPool( size_t totalThreads = 1 ) :
        m_totalThreads( ( totalThreads < 1 ) ? 1 : totalThreads ) {}
    ~ThreadPool() { Stop(); }

    ThreadPool( const ThreadPool& ) = delete;
    ThreadPool& operator=( const ThreadPool& ) = delete;

    void Stop()
    {
        if( m_workers.empty() )
        {
            return;
        }
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            m_stop = true;
            ++m_round;
        }
        m_cvWork.notify_all();
        for( auto& t : m_workers )
        {
            if( t.joinable() ) t.join();
        }
        m_workers.clear();
    }

    //
    // Invoke fn(i) for every i in [begin, end). The calling thread participates.
    // Falls back to a plain serial loop when there are no workers or the range is
    // smaller than one grain - so tiny work never pays synchronization cost.
    //
    template<class Fn>
    void ParallelFor( size_t begin, size_t end, size_t grain, Fn&& fn )
    {
        grain = ( grain < 1 ) ? 1 : grain;
        ensureStarted();

        if( m_workers.empty() || ( end - begin ) <= grain )
        {
            for( size_t i = begin; i < end; ++i )
            {
                fn( i );
            }
            return;
        }

        // Type-erase fn without heap allocation; fn outlives the round because this
        // function blocks until all chunks complete.
        using FnType = typename std::remove_reference<Fn>::type;
        m_ctx = static_cast<void*>( &fn );
        m_trampoline = []( void* ctx, size_t i ) { ( *static_cast<FnType*>( ctx ) )( i ); };

        {
            std::lock_guard<std::mutex> lock( m_mutex );
            m_next.store( begin, std::memory_order_relaxed );
            m_end          = end;
            m_grain        = grain;
            m_error        = nullptr;
            m_activeWorkers = m_workers.size();
            ++m_round;
        }
        m_cvWork.notify_all();

        // The caller pulls chunks too.
        runChunks();

        // Wait for workers to drain this round.
        {
            std::unique_lock<std::mutex> lock( m_mutex );
            m_cvDone.wait( lock, [this]() { return m_activeWorkers == 0; } );
        }

        m_ctx        = nullptr;
        m_trampoline = nullptr;

        if( m_error )
        {
            std::exception_ptr err = m_error;
            m_error = nullptr;
            std::rethrow_exception( err );
        }
    }

private:
    // Lazily create the configured number of persistent workers. Called only from
    // the owning Task's generation thread.
    void ensureStarted()
    {
        if( !m_workers.empty() || m_totalThreads <= 1 )
        {
            return;
        }

        m_stop = false;
        const size_t workerCount = m_totalThreads - 1;
        m_workers.reserve( workerCount );
        for( size_t i = 0; i < workerCount; ++i )
        {
            m_workers.emplace_back( [this]() { workerLoop(); } );
        }
    }

    void runChunks()
    {
        for( ;; )
        {
            const size_t i = m_next.fetch_add( m_grain, std::memory_order_relaxed );
            if( i >= m_end )
            {
                break;
            }
            const size_t j = std::min( i + m_grain, m_end );
            try
            {
                for( size_t k = i; k < j; ++k )
                {
                    m_trampoline( m_ctx, k );
                }
            }
            catch( ... )
            {
                std::lock_guard<std::mutex> lock( m_mutex );
                if( !m_error )
                {
                    m_error = std::current_exception();
                }
                // Stop everyone from grabbing more work this round.
                m_next.store( m_end, std::memory_order_relaxed );
                break;
            }
        }
    }

    void workerLoop()
    {
        size_t lastRound = 0;
        for( ;; )
        {
            {
                std::unique_lock<std::mutex> lock( m_mutex );
                m_cvWork.wait( lock, [this, lastRound]() { return m_stop || m_round != lastRound; } );
                if( m_stop )
                {
                    return;
                }
                lastRound = m_round;
            }

            runChunks();

            {
                std::lock_guard<std::mutex> lock( m_mutex );
                if( --m_activeWorkers == 0 )
                {
                    m_cvDone.notify_one();
                }
            }
        }
    }

    const size_t             m_totalThreads;
    std::vector<std::thread> m_workers;
    std::mutex               m_mutex;
    std::condition_variable  m_cvWork;
    std::condition_variable  m_cvDone;

    // Current job (set under lock before notify, read after the lock handshake).
    void  ( *m_trampoline )( void*, size_t ) = nullptr;
    void*               m_ctx     = nullptr;
    std::atomic<size_t> m_next{ 0 };
    size_t              m_end   = 0;
    size_t              m_grain = 1;

    size_t             m_round         = 0;
    size_t             m_activeWorkers = 0;
    bool               m_stop          = false;
    std::exception_ptr m_error;
};

}
