#include <iostream>
#include <ctime>
#include "cmdline.h"
using namespace std;

//
// Gets an unsigned integer out of an argument
// Returns 0 if unsuccessful; args normally don't use zeros so it is OK
//
unsigned int getUIntFromArg( wchar_t* text )
{
    if( wcslen( text ) < 4 || text[ 2 ] != PARAMETER_SEPARATOR )
    {
        return( 0 );
    }

    wstring numberAsString;
    numberAsString.assign( text + 3 );

    double number;
    if( !stringToNumber( numberAsString, number ) )
    {
        return( 0 );
    }

    int i = static_cast<int>( number );
    if( i < 0 ) i = 0;

    return ( static_cast<unsigned int> ( i ) );
}

//
// "space" and "tab" are special strings that can be used in some arguments
//
wchar_t getSpecialCharFromArg( wchar_t* text )
{
    if (wcslen( text ) < 4 || text[ 2 ] != PARAMETER_SEPARATOR)
    {
        return( 0 );
    }

    wchar_t c = 0;
    if( wcscmp( text + 3, L"space" ) == 0 ) c = L' ';
    if( wcscmp( text + 3, L"tab" )   == 0 ) c = L'\t';

    return (c);
}

//
//
//
wchar_t getCharFromArg( wchar_t* text )
{
    if( wcslen( text ) != 4 || text[ 2 ] != PARAMETER_SEPARATOR )
    {
        return ( 0 );
    }

    wchar_t c = text[ 3 ];
    if( c < 0 ) c = 0;

    return ( c );
}

//
//
//
wstring getStringFromArg( wchar_t* text )
{
    if( wcslen( text ) < 4 || text[ 2 ] != PARAMETER_SEPARATOR )
    {
        return ( wstring( L"" ) );
    }

    wstring ret( text );
    ret.erase( 0, 3 );

    return ( ret );
}

//
//
//
bool parseArg( wchar_t* text, CModelData& modelData )
{
    const wchar_t* UNKNOWN_OPTION_MSG = L"Unknown option:";

    // every argument is at least two char long and first character is '/' or '-'
    if( wcslen( text ) < 2 || ( text[ 0 ] != L'/' && text[ 0 ] != L'-' ) )
    {
        PrintMessage( InputDataError, UNKNOWN_OPTION_MSG, text );
        return( false );
    }

    wchar_t currentArg = towlower( text[ 1 ] );

    // are we seeing the same parameter again?
    if( modelData.ProvidedArguments.find( currentArg ) != modelData.ProvidedArguments.end() )
    {
        wstring opt = L"'";
        opt += currentArg;
        opt += L"'";
        PrintMessage( InputDataError, L"Option", opt.c_str(), L"was provided more than once" );
        return( false );
    }

    bool unknownOption = false;
    switch( currentArg )
    {
    case SWITCH_ORDER:
    {
        // Handle the "max" special case.
        if ( getStringFromArg( text ) == L"max" )
        {
            modelData.Order = MAXIMUM_ORDER;
            break;
        }
        unsigned int i = getUIntFromArg( text );
        if( i == 0 )
        {
            unknownOption = true;
            break;
        }
        modelData.Order = i;
        break;
    }
    case SWITCH_DELIMITER:
    {
        wchar_t c = getSpecialCharFromArg( text );
        if( c == 0 )
        {
            c = getCharFromArg( text );
            if( c == 0 )
            {
                unknownOption = true;
                break;
            }
        }
        modelData.ValuesDelim = c;
        break;
    }
    case SWITCH_ALIAS_DELIMITER:
    {
        wchar_t c = getSpecialCharFromArg( text );
        if( c == 0 )
        {
            c = getCharFromArg( text );
            if( c == 0 )
            {
                unknownOption = true;
                break;
            }
        }
        modelData.NamesDelim = c;
        break;
    }
    case SWITCH_NEGATIVE_VALUES:
    {
        wchar_t c = getCharFromArg( text );
        if( c == 0 )
        {
            unknownOption = true;
            break;
        }
        modelData.InvalidPrefix = c;
        break;
    }
    case SWITCH_SEED_FILE:
    {
        wstring s = getStringFromArg( text );
        if( s.empty() )
        {
            unknownOption = true;
            break;
        }
        modelData.RowSeedsFile = s;
        break;
    }
    case SWITCH_RANDOMIZE:
    {
        // /r
        if( wcslen( text ) == 2 )
        {
            modelData.RandSeed = (unsigned short) ( time( nullptr ) );
        }
        // /r:
        else if( wcslen( text ) == 3 )
        {
            unknownOption = true;
            break;
        }
        // /r:NNNN
        else
        {
            unsigned int i = getUIntFromArg( text );
            modelData.RandSeed = (unsigned short) i;
        }
        break;
    }
    case SWITCH_CASE_SENSITIVE:
    {
        if( wcslen( text ) == 2 )
        {
            modelData.CaseSensitive = true;
        }
        else
        {
            unknownOption = true;
            break;
        }
        break;
    }
    case SWITCH_STATISTICS:
    {
        if( wcslen( text ) == 2 )
        {
            modelData.Statistics = true;
        }
        else
        {
            unknownOption = true;
            break;
        }
        break;
    }
    case SWITCH_VERBOSE:
    {
        if( wcslen( text ) == 2 )
        {
            modelData.Verbose = true;
        }
        else
        {
            unknownOption = true;
            break;
        }
        break;
    }
    case SWITCH_PREVIEW:
    {
        if( wcslen( text ) == 2 )
        {
            modelData.GenerationMode = GenerationMode::Preview;
        }
        else
        {
            unknownOption = true;
            break;
        }
        break;
    }
    case SWITCH_APPROXIMATE:
    {
        // /x
        if( wcslen( text ) == 2 )
        {
            modelData.GenerationMode = GenerationMode::Approximate;
        }
        // /x:
        else if( wcslen( text ) == 3 )
        {
            unknownOption = true;
            break;
        }
        // /x:NNNN
        else
        {
            unsigned int i = getUIntFromArg( text );
            modelData.GenerationMode = GenerationMode::Approximate;
            modelData.MaxApproxTries = (unsigned short) i;
        }
        break;
    }
    default:
    {
        unknownOption = true;
        break;
    }
    }

    if( unknownOption )
    {
        PrintMessage( InputDataError, UNKNOWN_OPTION_MSG, text );
        return( false );
    }
    else
    {
        // add the current arg to the collection so we can discover dupes later
        modelData.ProvidedArguments.insert( currentArg );
        return( true );
    }
}

//
//
//
void showUsage()
{
    wcout << L"Pairwise Independent Combinatorial Testing" << endl << endl;
    wcout << L"Usage: pict model [options]"                << endl << endl;
    wcout << L"Options:" << endl;
    wcout << L" /" << charToStr( SWITCH_ORDER )             << L":N|max - Order of combinations (default: 2)" << endl;
    wcout << L" /" << charToStr( SWITCH_DELIMITER )         << L":C     - Separator for values  (default: ,)" << endl;
    wcout << L" /" << charToStr( SWITCH_ALIAS_DELIMITER )   << L":C     - Separator for aliases (default: |)" << endl;
    wcout << L" /" << charToStr( SWITCH_NEGATIVE_VALUES )   << L":C     - Negative value prefix (default: ~)" << endl;
    wcout << L" /" << charToStr( SWITCH_SEED_FILE )         << L":file  - File with seeding rows"             << endl;
    wcout << L" /" << charToStr( SWITCH_RANDOMIZE )         << L"[:N]   - Randomize generation, N - seed"     << endl;
    wcout << L" /" << charToStr( SWITCH_CASE_SENSITIVE )    << L"       - Case-sensitive model evaluation"    << endl;
    wcout << L" /" << charToStr( SWITCH_STATISTICS )        << L"       - Show model statistics"              << endl;
    // there are hidden parameters:
    // wcout << L" /x  - Approximate generation (best effort, might not cover all combinations)" << endl;
    // wcout << L" /p  - Preview generation (only a few test cases)"                             << endl;
    // wcout << L" /v  - Verbose mode"                                                           << endl;
}

//
//
//
bool ParseArgs
    (
    IN     int         argc,
    IN     wchar_t*    args[],
    IN OUT CModelData& modelData
    )
{
    if( argc < 2 )
    {
        showUsage();
        return( false );
    }

    if( wcscmp( args[ 1 ], L"/?" ) == 0
     || wcscmp( args[ 1 ], L"-?" ) == 0 )
    {
        showUsage();
        return( false );
    }

    // process all arguments starting from the second one; first one must be a model file name
    int ii = 2;
    while( ii < argc )
    {
        if( !parseArg( args[ ii ], modelData ) )
        {
            return( false );
        }
        ii++;
    }

    return( true );
}
