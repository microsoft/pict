/* Define __cdecl for non-Microsoft compilers */
#if ( !defined( _MSC_VER ) && !defined( __cdecl ))
#define __cdecl
#endif

#include <atomic>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <iostream>
#include <limits>
#include <locale>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
using namespace std;

#include "cmdline.h"
#include "gcd.h"
#include "strings.h"
using namespace pictcli_gcd;

//
// Resolves the worker thread count for best-of-N search.
// Precedence: explicit /t:N (requested != 0) > PICT_THREADS env var > hardware_concurrency().
//
static size_t resolveThreadCount( size_t requested )
{
    if( requested != 0 )
    {
        return requested;
    }

    size_t envThreads = 0;
#if defined( _MSC_VER )
    char*  buffer = nullptr;
    size_t length = 0;
    if( _dupenv_s( &buffer, &length, "PICT_THREADS" ) == 0 && buffer != nullptr )
    {
        envThreads = static_cast<size_t>( strtoul( buffer, nullptr, 10 ) );
    }
    if( buffer != nullptr )
    {
        free( buffer );
    }
#else
    const char* raw = getenv( "PICT_THREADS" );
    if( raw != nullptr )
    {
        envThreads = static_cast<size_t>( strtoul( raw, nullptr, 10 ) );
    }
#endif
    if( envThreads != 0 )
    {
        return envThreads;
    }

    const unsigned int detected = thread::hardware_concurrency();
    return ( detected == 0 ) ? static_cast<size_t>( 1 ) : static_cast<size_t>( detected );
}

//
// Runs one full generation for a single seed and returns the resulting suite size.
// Returns SIZE_MAX if generation fails (e.g. a seed-dependent generation failure),
// so that successful seeds are always preferred during reduction.
//
static size_t trialSuiteSize( const CModelData& baseModelData, unsigned short seed )
{
    CModelData modelData = baseModelData; // deep copy; GcdPointers are null pre-generation
    modelData.RandSeed      = seed;
    modelData.Verbose       = false;      // keep per-trial runs quiet and deterministic
    modelData.EngineThreads = 1;          // trials parallelize across seeds, not inside the engine

    GcdRunner runner( modelData );
    if( runner.Generate() != ErrorCode::ErrorCode_Success )
    {
        return numeric_limits<size_t>::max();
    }
    return runner.GetResult().TestCases.size();
}

//
// Best-of-N seed search.
// Tries 'bestOf' independent seeds derived deterministically from the base seed and
// returns the seed that produced the smallest suite. Ties are broken by the earliest
// seed in the sequence, so the winner is independent of how trials are scheduled.
// Each trial is a fully independent generation (own model copy, task, and RNG), which
// is what makes parallel execution safe and bit-for-bit reproducible.
//
static unsigned short findBestSeed
    (
    IN  const CModelData& modelData,
    IN  size_t            bestOf,
    IN  size_t            threadCount,
    OUT size_t&           bestRows
    )
{
    const unsigned short baseSeed = modelData.RandSeed;

    // Pre-compute every trial seed up front so the sequence (and thus the chosen
    // winner) is identical regardless of thread scheduling.
    vector<unsigned short> seeds( bestOf );
    for( size_t i = 0; i < bestOf; ++i )
    {
        seeds[ i ] = static_cast<unsigned short>( baseSeed + i );
    }

    vector<size_t> rows( bestOf, numeric_limits<size_t>::max() );

    const size_t workers = ( threadCount <= 1 ) ? 1 : min( threadCount, bestOf );
    if( workers <= 1 )
    {
        for( size_t i = 0; i < bestOf; ++i )
        {
            rows[ i ] = trialSuiteSize( modelData, seeds[ i ] );
        }
    }
    else
    {
        atomic<size_t> nextIndex( 0 );
        auto worker = [&]()
        {
            for( ;; )
            {
                const size_t i = nextIndex.fetch_add( 1 );
                if( i >= bestOf ) break;
                rows[ i ] = trialSuiteSize( modelData, seeds[ i ] );
            }
        };

        vector<thread> pool;
        pool.reserve( workers );
        for( size_t t = 0; t < workers; ++t )
        {
            pool.emplace_back( worker );
        }
        for( size_t t = 0; t < pool.size(); ++t )
        {
            pool[ t ].join();
        }
    }

    // Deterministic reduction: smallest suite wins, ties broken by earliest seed.
    size_t bestIndex = 0;
    for( size_t i = 1; i < bestOf; ++i )
    {
        if( rows[ i ] < rows[ bestIndex ] )
        {
            bestIndex = i;
        }
    }

    bestRows = rows[ bestIndex ];
    return seeds[ bestIndex ];
}

//
//
//
void printTimeDifference( time_t start, time_t end )
{
    int diff = static_cast<int> ( difftime( end, start ) );

    // hours
    int hrs = diff / 3600;
    wcout << hrs << L":";

    // minutes
    diff -= hrs * 3600;
    int mins = diff / 60;
    wcout << ( mins < 10 ? L"0" : L"" ) << mins << L":";

    // seconds
    diff -= mins * 60;
    wcout << ( diff < 10 ? L"0" : L"" ) << diff;

    wcout << endl;
}

//
//
//
int __cdecl execute
    (
    IN     int      argc,
    IN     wchar_t* args[],
    IN OUT wstring& output
    )
{
    time_t start = time( nullptr );

    CModelData modelData;

    if( !ParseArgs( argc, args, modelData ) )
    {
        return( (int)ErrorCode::ErrorCode_BadOption );
    }

    if( !modelData.ReadModel( wstring( args[ 1 ] )))
    {
        return( (int)ErrorCode::ErrorCode_BadModel );
    }

    if( !modelData.ReadRowSeedFile( modelData.RowSeedsFile ))
    {
        return( (int)ErrorCode::ErrorCode_BadRowSeedFile );
    }

    // Resolve worker threads once. When best-of-N is active, the threads parallelize
    // independent seed trials (engine stays serial per trial); otherwise they
    // parallelize inside the single generation.
    const size_t resolvedThreads = resolveThreadCount( modelData.ThreadCount );

    // Best-of-N: try several seeds and keep the smallest suite. The search runs
    // independent generations (optionally in parallel); the winning seed is then
    // used for the single authoritative generation below, so output, statistics,
    // verbose, and JSON formatting all follow the exact same path as a normal run.
    if( modelData.BestOf > 1 )
    {
        size_t bestRows = 0;
        unsigned short bestSeed = findBestSeed( modelData,
                                                modelData.BestOf,
                                                resolvedThreads,
                                                bestRows );
        modelData.RandSeed = bestSeed;

        wcerr << L"Best of " << modelData.BestOf << L" seeds: seed "
              << static_cast<int>( bestSeed ) << L" produced ";
        if( bestRows == numeric_limits<size_t>::max() )
        {
            wcerr << L"no result";
        }
        else
        {
            wcerr << bestRows << L" rows";
        }
        wcerr << endl;
    }

    // The single authoritative generation may use the engine worker pool.
    modelData.EngineThreads = resolvedThreads;

    GcdRunner gcdRunner( modelData );

    ErrorCode err = gcdRunner.Generate();
    if( err != ErrorCode::ErrorCode_Success )
    {
        return (int)err;
    }

    time_t end = time( nullptr );

    // if r has been provided then print out the seed
    // TODO: change to not use SWITCH_RANDOMIZE const
    if( modelData.ProvidedArguments.find( SWITCH_RANDOMIZE ) != modelData.ProvidedArguments.end() )
    {
        wcerr << L"Used seed: " << static_cast<int>( modelData.RandSeed ) << endl;
    }

    CResult result = gcdRunner.GetResult();

    if( modelData.Statistics )
    {
        modelData.PrintStatistics();
        result.PrintStatistics();
        PrintStatisticsCaption( wstring( L"Generation time" ));
        printTimeDifference( start, end );
    }
    else
    {
        result.PrintConstraintWarnings();

        wostringstream outputStream;
        if( modelData.Format == L"json" )
        {
            result.PrintOutputJson( modelData, outputStream );
        }
        else
        {
            result.PrintOutput( modelData, outputStream );
        }
        output.append( outputStream.str() );
    }

    return( (int)ErrorCode::ErrorCode_Success );
}

//
//
//
int __cdecl wmain
    (
    IN int      argc,
    IN wchar_t* args[]
    )
{
    // Align all wide I/O with the current environment locale (UTF-8 on modern systems)
    try
    {
        std::locale loc( "" );
        std::locale::global( loc );
        std::wcout.imbue( loc );
        std::wcerr.imbue( loc );
    }
    catch( const std::runtime_error& )
    {
        // Fall back to the classic locale if the environment locale is unavailable
        std::locale::global( std::locale::classic() );
    }

    wstring output;
    int ret = execute( argc, args, output );

    std::wcout << output;

    return ret;
}

#if !defined(_MSC_VER)
//
// Gcc doesn't understand wchar_t args
// This entry point is a workaround for compiling with a non-MS compiler
//
int main
    (
    IN int   argc,
    IN char* args[]
    )
{
    // Use "C.UTF-8" locale for multi-byte character I/O everywhare
    // If "C.UTF-8" locale is not supported, use classic "C" locale as fallback.
    std::locale loc;
    try
    {
        loc = std::locale("C.UTF-8");
    }
    catch ( const std::runtime_error&)
    {
        loc = std::locale::classic();
    }
    std::locale::global(loc);

    // convert all args to wchar_t's
    wchar_t** wargs = new wchar_t*[ argc ];
    for ( int ii = 0; ii < argc; ++ii )
    {
        size_t len = strlen( args[ ii ] );
        wargs[ ii ] = new wchar_t[ len + 1 ];
        size_t jj;
        for( jj = 0; jj < len; ++jj )
        {
            wargs[ ii ][ jj ] = (wchar_t) args[ ii ][ jj ];
        }
        wargs[ ii ][ jj ] = L'\0';
    }

    // invoke wmain
    int ret = wmain( argc, wargs );

    // clean up
    for ( int ii = 0; ii < argc; ++ii )
    {
        delete[] wargs[ ii ];
    }
    delete[] wargs;

    return( ret );
}
#endif
