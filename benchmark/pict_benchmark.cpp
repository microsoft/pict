/* Define __cdecl for non-Microsoft compilers */
#if ( !defined( _MSC_VER ) && !defined( __cdecl ))
#define __cdecl
#endif

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "pictapi.h"

using namespace std;

namespace
{
struct BenchmarkPart
{
    size_t ValueCount;
    size_t ParameterCount;
};

struct BenchmarkDefinition
{
    string Name;
    vector<string> Aliases;
    vector<BenchmarkPart> Parts;
    int BestKnownRows;
    int PictReferenceRows;
};

struct BenchmarkResult
{
    string Name;
    size_t Runs;
    size_t BestRows;
    unsigned int BestSeed;
    int BestKnownRows;
    int PictReferenceRows;
};

string FormatModelShape( const BenchmarkDefinition& benchmark )
{
    ostringstream stream;
    for( size_t index = 0; index < benchmark.Parts.size(); ++index )
    {
        if( index != 0 )
        {
            stream << ' ';
        }

        stream << benchmark.Parts[ index ].ValueCount << '^' << benchmark.Parts[ index ].ParameterCount;
    }

    return stream.str();
}

class ScopedTask
{
public:
    explicit ScopedTask( PICT_HANDLE handle ) : _handle( handle ) {}
    ~ScopedTask()
    {
        if( _handle != nullptr )
        {
            PictDeleteTask( _handle );
        }
    }

    PICT_HANDLE Get() const { return( _handle ); }

private:
    PICT_HANDLE _handle;
};

class ScopedModel
{
public:
    explicit ScopedModel( PICT_HANDLE handle ) : _handle( handle ) {}
    ~ScopedModel()
    {
        if( _handle != nullptr )
        {
            PictDeleteModel( _handle );
        }
    }

    PICT_HANDLE Get() const { return( _handle ); }

private:
    PICT_HANDLE _handle;
};

class ScopedResultBuffer
{
public:
    explicit ScopedResultBuffer( PICT_RESULT_ROW handle ) : _handle( handle ) {}
    ~ScopedResultBuffer()
    {
        if( _handle != nullptr )
        {
            PictFreeResultBuffer( _handle );
        }
    }

    PICT_RESULT_ROW Get() const { return( _handle ); }

private:
    PICT_RESULT_ROW _handle;
};

const BenchmarkDefinition g_benchmarks[] =
{
    { "3^4", { "3^4", "3_4" }, { { 3, 4 } }, 9, 9 },
    { "3^13", { "3^13", "3_13" }, { { 3, 13 } }, 15, 18 },
    { "2^100", { "2^100", "2_100" }, { { 2, 100 } }, 10, 15 },
    { "10^20", { "10^20", "10_20" }, { { 10, 20 } }, 180, 210 },
    { "4^15 3^17 2^29", { "4^15 3^17 2^29", "4_15__3_17__2_29" }, { { 4, 15 }, { 3, 17 }, { 2, 29 } }, 29, 37 },
    { "4^1 3^39 2^35", { "4^1 3^39 2^35", "4_1__3_39__2_35" }, { { 4, 1 }, { 3, 39 }, { 2, 35 } }, 21, 27 }
};

string NormalizeBenchmarkName( const string& text )
{
    string normalized;
    normalized.reserve( text.size() );

    for( string::const_iterator it = text.begin(); it != text.end(); ++it )
    {
        if( *it == ' ' || *it == '\t' || *it == '\n' || *it == '\r' )
        {
            continue;
        }

        normalized.push_back( static_cast<char>( tolower( static_cast<unsigned char>( *it ))));
    }

    return normalized;
}

bool TryParseUnsigned( const string& text, size_t& value )
{
    if( text.empty() )
    {
        return false;
    }

    char* end = nullptr;
    const unsigned long parsed = strtoul( text.c_str(), &end, 10 );
    if( end == nullptr || *end != '\0' )
    {
        return false;
    }

    value = static_cast<size_t>( parsed );
    return true;
}

bool TryParseBenchmarkSpec( const string& spec, BenchmarkDefinition& benchmark )
{
    string rewritten = spec;
    for( size_t index = 0; index < rewritten.size(); ++index )
    {
        if( rewritten[ index ] == ',' || rewritten[ index ] == ';' )
        {
            rewritten[ index ] = ' ';
        }
    }

    istringstream stream( rewritten );
    string token;
    vector<BenchmarkPart> parts;
    ostringstream name;
    bool first = true;

    while( stream >> token )
    {
        const string::size_type caret = token.find( '^' );
        if( caret == string::npos || caret == 0 || caret == token.size() - 1 )
        {
            return false;
        }

        size_t valueCount = 0;
        size_t parameterCount = 0;
        if( !TryParseUnsigned( token.substr( 0, caret ), valueCount ) ||
            !TryParseUnsigned( token.substr( caret + 1 ), parameterCount ) ||
            valueCount == 0 ||
            parameterCount == 0 )
        {
            return false;
        }

        parts.push_back( BenchmarkPart{ valueCount, parameterCount } );

        if( !first )
        {
            name << ' ';
        }
        name << valueCount << '^' << parameterCount;
        first = false;
    }

    if( parts.empty() )
    {
        return false;
    }

    benchmark.Name = name.str();
    benchmark.Aliases.clear();
    benchmark.Parts = parts;
    benchmark.BestKnownRows = -1;
    benchmark.PictReferenceRows = -1;
    return true;
}

bool TryResolveBenchmark( const string& text, BenchmarkDefinition& benchmark )
{
    const string normalized = NormalizeBenchmarkName( text );
    for( size_t index = 0; index < sizeof( g_benchmarks ) / sizeof( g_benchmarks[ 0 ] ); ++index )
    {
        const BenchmarkDefinition& candidate = g_benchmarks[ index ];
        for( size_t aliasIndex = 0; aliasIndex < candidate.Aliases.size(); ++aliasIndex )
        {
            if( NormalizeBenchmarkName( candidate.Aliases[ aliasIndex ] ) == normalized )
            {
                benchmark = candidate;
                return true;
            }
        }
    }

    return TryParseBenchmarkSpec( text, benchmark );
}

void CheckRetCode( PICT_RET_CODE ret, const string& action )
{
    if( ret == PICT_SUCCESS )
    {
        return;
    }

    ostringstream message;
    message << action << " failed with error 0x" << hex << ret;
    throw runtime_error( message.str() );
}

size_t RunSingleTrial( const BenchmarkDefinition& benchmark, unsigned int seed )
{
    ScopedTask task( PictCreateTask() );
    ScopedModel model( PictCreateModel( seed ) );

    if( task.Get() == nullptr || model.Get() == nullptr )
    {
        throw runtime_error( "Unable to allocate PICT task or model" );
    }

    PictSetRootModel( task.Get(), model.Get() );

    for( size_t partIndex = 0; partIndex < benchmark.Parts.size(); ++partIndex )
    {
        const BenchmarkPart& part = benchmark.Parts[ partIndex ];
        for( size_t parameterIndex = 0; parameterIndex < part.ParameterCount; ++parameterIndex )
        {
            if( PictAddParameter( model.Get(), part.ValueCount, PICT_PAIRWISE_GENERATION ) == nullptr )
            {
                throw runtime_error( "Unable to add benchmark parameter" );
            }
        }
    }

    CheckRetCode( PictGenerate( task.Get() ), "Generation" );

    ScopedResultBuffer row( PictAllocateResultBuffer( task.Get() ) );
    if( row.Get() == nullptr )
    {
        throw runtime_error( "Unable to allocate result buffer" );
    }

    size_t resultCount = 0;
    PictResetResultFetching( task.Get() );
    while( PictGetNextResultRow( task.Get(), row.Get() ) != 0 )
    {
        ++resultCount;
    }

    return resultCount;
}

BenchmarkResult RunBenchmark( const BenchmarkDefinition& benchmark, size_t runs, unsigned int& seedState )
{
    BenchmarkResult result;
    result.Name = benchmark.Name;
    result.Runs = runs;
    result.BestRows = numeric_limits<size_t>::max();
    result.BestSeed = 0;
    result.BestKnownRows = benchmark.BestKnownRows;
    result.PictReferenceRows = benchmark.PictReferenceRows;

    for( size_t run = 0; run < runs; ++run )
    {
        seedState = seedState * 1664525u + 1013904223u;
        const unsigned int trialSeed = seedState;
        const size_t rows = RunSingleTrial( benchmark, trialSeed );

        cerr << "[trial " << ( run + 1 ) << "/" << runs << "] "
             << "benchmark=\"" << benchmark.Name << "\" "
             << "model=\"" << FormatModelShape( benchmark ) << "\" "
             << "seed=" << trialSeed << ' '
             << "rows=" << rows << endl;

        if( rows < result.BestRows )
        {
            result.BestRows = rows;
            result.BestSeed = trialSeed;
        }
    }

    return result;
}

string ToText( int value )
{
    if( value < 0 )
    {
        return "-";
    }

    ostringstream stream;
    stream << value;
    return stream.str();
}

void PrintUsage( const char* programName )
{
    cerr << "Usage: " << programName << " [--runs N] [--seed N] [--benchmark SPEC]..." << endl;
    cerr << "       " << programName << " --list" << endl;
    cerr << endl;
    cerr << "Runs the standard pairwise benchmark scenarios repeatedly with different seeds" << endl;
    cerr << "and reports the smallest generated suite for each selected benchmark." << endl;
    cerr << endl;
    cerr << "Examples:" << endl;
    cerr << "  " << programName << " --runs 1000" << endl;
    cerr << "  " << programName << " --runs 250 --benchmark 3^13 --benchmark \"4^15 3^17 2^29\"" << endl;
    cerr << "  " << programName << " --runs 500 --benchmark \"6^20\"" << endl;
}

void PrintBenchmarks()
{
    cout << "Built-in benchmarks:" << endl;
    for( size_t index = 0; index < sizeof( g_benchmarks ) / sizeof( g_benchmarks[ 0 ] ); ++index )
    {
        const BenchmarkDefinition& benchmark = g_benchmarks[ index ];
        cout << "  " << benchmark.Name
             << "  (best known: " << benchmark.BestKnownRows
             << ", pairwise.org PICT: " << benchmark.PictReferenceRows << ")" << endl;
    }
}

void PrintResults( const vector<BenchmarkResult>& results, size_t runs, unsigned int baseSeed )
{
    size_t benchmarkWidth = string( "Benchmark" ).size();
    for( size_t index = 0; index < results.size(); ++index )
    {
        benchmarkWidth = max( benchmarkWidth, results[ index ].Name.size() );
    }

    cout << "Runs per benchmark: " << runs << endl;
    cout << "Base seed: " << baseSeed << endl;
    cout << endl;

    cout << left
         << setw( static_cast<int>( benchmarkWidth )) << "Benchmark" << "  "
         << right << setw( 6 ) << "Runs" << "  "
         << setw( 6 ) << "Best" << "  "
         << setw( 10 ) << "Best seed" << "  "
         << setw( 10 ) << "Best known" << "  "
         << setw( 8 ) << "PICT ref" << "  "
         << setw( 6 ) << "Delta" << endl;

    cout << string( benchmarkWidth + 2 + 6 + 2 + 6 + 2 + 10 + 2 + 10 + 2 + 8 + 2 + 6, '-' ) << endl;

    for( size_t index = 0; index < results.size(); ++index )
    {
        const BenchmarkResult& result = results[ index ];
        string delta = "-";
        if( result.PictReferenceRows >= 0 )
        {
            delta = ToText( static_cast<int>( result.BestRows ) - result.PictReferenceRows );
        }

        cout << left
             << setw( static_cast<int>( benchmarkWidth )) << result.Name << "  "
             << right << setw( 6 ) << result.Runs << "  "
             << setw( 6 ) << result.BestRows << "  "
             << setw( 10 ) << result.BestSeed << "  "
             << setw( 10 ) << ToText( result.BestKnownRows ) << "  "
             << setw( 8 ) << ToText( result.PictReferenceRows ) << "  "
             << setw( 6 ) << delta << endl;
    }
}
}

int __cdecl main( int argc, char* argv[] )
{
    try
    {
        size_t runs = 1000;
        unsigned int baseSeed = static_cast<unsigned int>( time( nullptr ));
        vector<BenchmarkDefinition> selectedBenchmarks;

        for( int index = 1; index < argc; ++index )
        {
            const string argument = argv[ index ];

            if( argument == "--help" || argument == "-h" )
            {
                PrintUsage( argv[ 0 ] );
                return 0;
            }

            if( argument == "--list" )
            {
                PrintBenchmarks();
                return 0;
            }

            if( argument == "--runs" || argument == "-n" )
            {
                if( index + 1 >= argc || !TryParseUnsigned( argv[ ++index ], runs ) || runs == 0 )
                {
                    throw runtime_error( "Expected a positive integer after --runs" );
                }
                continue;
            }

            if( argument == "--seed" )
            {
                size_t parsedSeed = 0;
                if( index + 1 >= argc || !TryParseUnsigned( argv[ ++index ], parsedSeed ) )
                {
                    throw runtime_error( "Expected an integer after --seed" );
                }

                baseSeed = static_cast<unsigned int>( parsedSeed );
                continue;
            }

            if( argument == "--benchmark" || argument == "-b" )
            {
                if( index + 1 >= argc )
                {
                    throw runtime_error( "Expected a benchmark name or spec after --benchmark" );
                }

                BenchmarkDefinition benchmark;
                if( !TryResolveBenchmark( argv[ ++index ], benchmark ) )
                {
                    throw runtime_error( string( "Unknown benchmark: " ) + argv[ index ] );
                }

                selectedBenchmarks.push_back( benchmark );
                continue;
            }

            throw runtime_error( string( "Unknown argument: " ) + argument );
        }

        if( selectedBenchmarks.empty() )
        {
            selectedBenchmarks.assign( g_benchmarks, g_benchmarks + sizeof( g_benchmarks ) / sizeof( g_benchmarks[ 0 ] ));
        }

        vector<BenchmarkResult> results;
        results.reserve( selectedBenchmarks.size() );

        unsigned int seedState = baseSeed;
        for( size_t index = 0; index < selectedBenchmarks.size(); ++index )
        {
            results.push_back( RunBenchmark( selectedBenchmarks[ index ], runs, seedState ));
        }

        PrintResults( results, runs, baseSeed );
        return 0;
    }
    catch( const exception& ex )
    {
        cerr << "pict-benchmark: " << ex.what() << endl;
        return 1;
    }
}
