/* Define __cdecl for non-Microsoft compilers */
#if ( !defined( _MSC_VER ) && !defined( __cdecl ))
#define __cdecl
#endif

#include <ctime>
#include <cstring>
using namespace std;

#include "cmdline.h"
#include "gcd.h"
using namespace pictcli_gcd;

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
    time_t start = time( NULL );

    CModelData modelData;

    if( !ParseArgs( argc, args, modelData ) )
    {
        return( ErrorCode_BadOption );
    }

    if( !modelData.ReadModel( wstring( args[ 1 ] )))
    {
        return( ErrorCode_BadModel );
    }

    if( !modelData.ReadRowSeedFile( modelData.RowSeedsFile ))
    {
        return( ErrorCode_BadRowSeedFile );
    }

    GcdRunner gcdRunner( modelData );

    ErrorCode err = gcdRunner.Generate();
    if( err != ErrorCode_Success )
    {
        return err;
    }

    time_t end = time( NULL );

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
        result.PrintOutput( modelData, outputStream );
        output.append( outputStream.str() );
    }

    return( ErrorCode_Success );
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
    wstring output;
    int ret = execute( argc, args, output );
    wcout << output;

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
        delete wargs[ ii ];
    }
    delete wargs;

    return( ret );
}
#endif
