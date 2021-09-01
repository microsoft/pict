#include <sstream>
#include <string>
#include <algorithm>
#include <functional> 
#include <cctype>
#include <cwchar>
#include <cwctype>
#include <cassert>
#include "strings.h"
using namespace std;

//
//
//
wstring::iterator findFirstNonWhitespace( const wstring::iterator begin, const wstring::iterator end )
{
    return( find_if( begin, end,
                     [](const wint_t c) {
                         return ! std::iswspace(c);
                     } ) );
}

//
//
//
void toUpper( wstring& s )
{
    transform( s.begin(), s.end(), s.begin(), towupper );
}

//
//
//
bool lineIsEmpty( wstring &line )
{
    return( trim( line ).empty() );
}

//
//
//
int stringCompare( const wstring& s1, const wstring& s2, bool caseSensitive )
{
    if( !caseSensitive )
    {
        wstring text1( s1 );
        wstring text2( s2 );

        toUpper( text1 );
        toUpper(text2);

        return text1.compare( text2 );
    }
    else
    {
        return s1.compare( s2 );
    }
}

//
//
//
wstring trim( wstring text )
{
    text.erase( text.begin(),
                find_if( text.begin(), text.end(),
                         [](const wint_t c) {
                             return ! std::iswspace(c);
                         } ) );
    text.erase( find_if( text.rbegin(), text.rend(),
                         [](const wint_t c) {
                             return ! std::iswspace(c);
                         } ).base(),
                text.end() );
    return text;
}

//
//
//
void split( wstring& text, wchar_t delimiter, wstrings& items )
{
    wstring::size_type start, end, len;

    end = wstring::npos;
    while( true )
    {
        start = end + 1;
        end = text.find( delimiter, start );

        if( end == wstring::npos )
            len = text.size() + 1;
        else
            len = end - start;

        wstring item = wstring( text.substr( start, len ) );
        items.push_back( item );

        if( end == wstring::npos ) break;
    }
}

//
//
//
double stringToNumber( wstring& text )
{
    wstring::size_type endptr;
    double number = std::stod( text, &endptr );
    // the conversion is successful only when no text remains after the number
    if( endptr != text.length() )
    {
        throw new std::invalid_argument( "stringToNumber called on a text that contains characters after the number" );
    }
    return( number );
}

//
//
//
bool stringToNumber( wstring& text, double& number )
{
    try
    {
        number = stringToNumber( text );
        return( true );
    }
    catch( ... )
    {
        assert( true );
        return( false );
    }
}


//
//
//
bool textContainsNumber( IN wstring& text )
{
    double d;
    return( stringToNumber( text, d ) );
}

//
//
//
bool patternMatch( const wchar_t* wstrPattern, const wchar_t* wstrString )
{
    wchar_t chSrc, chPat;

    while( *wstrString )
    {
        chSrc = *wstrString;
        chPat = *wstrPattern;

        if( chPat == L'*' )
        {
            // skip all aterisks that are grouped together
            while( wstrPattern[ 1 ] == L'*' )
            {
                wstrPattern++;
            }

            // sheck if aterisk is at the end. If so, we have a match already.
            chPat = wstrPattern[ 1 ];
            if( !chPat )
            {
                return true;
            }

            // otherwise check if next pattern char matches current char
            if( chPat == chSrc || chPat == L'?' )
            {
                // do recursive check for rest of pattern
                wstrPattern++;
                if( patternMatch( wstrPattern, wstrString ) )
                {
                    return true;
                }

                // no, that didn't work, stick with asterisk
                wstrPattern--;
            }

            // allow any character and continue
            wstrString++;
            continue;
        }

        if( chPat != L'?' )
        {
            // if the next pattern character is not a question mark,
            // src and pat must be identical.
            if( chSrc != chPat )
            {
                return false;
            }
        }

        // advance when pattern character matches string character
        wstrPattern++;
        wstrString++;
    }

    // fail when there is more pattern and pattern does not end in an asterisk
    chPat = *wstrPattern;
    if( chPat && ( chPat != L'*' || wstrPattern[ 1 ] ) )
    {
        return false;
    }

    return true;
}

//
//
//
bool patternMatch( wstring pattern, wstring text )
{
    const wchar_t* wstrPattern = pattern.c_str();
    const wchar_t* wstrText = text.c_str();

    return patternMatch( wstrPattern, wstrText );
}

//
//
//
wstring charToStr( wchar_t c )
{
    wchar_t arr[ 2 ] = { c, 0 };
    return( wstring( arr ) );
}

//
//
//
wstring charArrToStr( const wchar_t* c )
{
    return( wstring( c ) );
}

//
//
//
string wideCharToAnsi( const wstring& text )
{
    string ansiText;
    ansiText.reserve( text.size() );
    for( auto c : text )
    {
        ansiText += static_cast< char > (c);
    }
    return( ansiText );
}

//
//
//
EncodingType getEncodingType( wstring& text )
{
    // UTF8
    if( text.size() >= 3
     && text[0] == (wchar_t)0xEF
     && text[1] == (wchar_t)0xBB
     && text[2] == (wchar_t)0xBF )
    {
        text.erase( 0, 3 );
        return( EncodingType::UTF8 );
    }

    // UTF32_BigEndian
    if( text.size() >= 4
     && text[0] == (wchar_t)0x00
     && text[1] == (wchar_t)0x00
     && text[2] == (wchar_t)0xFE
     && text[3] == (wchar_t)0xFF )
    {
        text.erase( 0, 4 );
        return( EncodingType::UTF32_BigEndian );
    }

    // UTF32_LittleEndian
    if( text.size() >= 4
     && text[0] == (wchar_t)0xFF
     && text[1] == (wchar_t)0xFE
     && text[2] == (wchar_t)0x00
     && text[3] == (wchar_t)0x00 )
    {
        text.erase( 0, 4 );
        return( EncodingType::UTF32_LittleEndian );
    }

    // UTF16_BigEndian
    if( text.size() >= 2
     && text[0] == (wchar_t)0xFE
     && text[1] == (wchar_t)0xFF )
    {
        text.erase( 0, 2 );
        return( EncodingType::UTF16_BigEndian );
    }

    // UTF16_LittleEndian
    if( text.size() >= 2
     && text[0] == (wchar_t)0xFF
     && text[1] == (wchar_t)0xFE )
    {
        text.erase( 0, 2 );
        return( EncodingType::UTF16_LittleEndian );
    }

    return( EncodingType::ANSI );
}

//
//
//
void setEncodingType
    (
    IN     EncodingType type,
    IN OUT wstring&     text
    )
{
    assert( text.empty() );

    switch( type )
    {
    case EncodingType::ANSI:
        break;

    case EncodingType::UTF8:
        text += (wchar_t)0xEF;
        text += (wchar_t)0xBB;
        text += (wchar_t)0xBF;
        break;

    case EncodingType::UTF16_BigEndian:
    case EncodingType::UTF16_LittleEndian:
    case EncodingType::UTF32_BigEndian:
    case EncodingType::UTF32_LittleEndian:
        assert(false);
        break;
    }
}

//
// Predicates for unique() and sort()
//
bool stringCaseSensitiveLess( wstring s1, wstring s2 )
{
    return( stringCompare( s1, s2, true ) < 0 );
}

bool stringCaseInsensitiveLess( wstring s1, wstring s2 )
{
    return( stringCompare( s1, s2, false ) < 0 );
}

bool stringCaseSensitiveEquals( wstring s1, wstring s2 )
{
    return( stringCompare( s1, s2, true ) == 0 );
}

bool stringCaseInsensitiveEquals( wstring s1, wstring s2 )
{
    return( stringCompare( s1, s2, false ) == 0 );
}
