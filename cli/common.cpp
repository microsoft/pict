#include <sstream>
#include <iostream>
#include <assert.h>
#include "common.h"
using namespace std;

//
//
//
void PrintMessage
    (
    IN MsgType  type,
    IN const wchar_t* text1,
    IN const wchar_t* text2,
    IN const wchar_t* text3
    )
{
    switch ( type )
    {
    case InputDataError:
        wcerr << L"Input Error: ";
        break;

    case InputDataWarning:
        wcerr << L"Input Warning: ";
        break;

    case GcdError:
        wcerr << L"Generation Error: ";
        break;

    case RowSeedsError:
        wcerr << L"Seeding Error: ";
        break;

    case SystemError:
        wcerr << L"System Error: ";
        break;

    case ConstraintsWarning:
        wcerr << L"Constraints Warning: ";
        break;

    case RowSeedsWarning:
        wcerr << L"Seeding Warning: ";
        break;

    default:
        assert( false );
        break;
    }

    wcerr << text1;
    if ( text2 != nullptr ) wcerr << L" " << text2;
    if ( text3 != nullptr ) wcerr << L" " << text3;
    wcerr << endl;
}

//
//
//
void PrintLogHeader( wstring title )
{
    const size_t  TOTAL_LENGTH = 65;
    const wchar_t HEADER_CHAR = L'~';

    size_t n = ( TOTAL_LENGTH - title.size() - 2 ) / 2;

    wstring header;

    header.append( n, HEADER_CHAR );
    header += L' ';
    header.append( title.begin(), title.end() );
    header += L' ';
    header.append( n, HEADER_CHAR );
    header.append( TOTAL_LENGTH - header.size(), HEADER_CHAR );
    header += L'\n';

    wcerr << header;
}

//
//
//
void PrintStatisticsCaption( const wstring& caption )
{
    const size_t PADDING_SIZE = 15;
    assert( PADDING_SIZE >= caption.size() );

    wstring padding( PADDING_SIZE - caption.size(), L' ' );
    wcout << caption << L":" << padding;
}
