#pragma once

#include <vector>
#include <string>

#include "common.h"

typedef std::vector< std::wstring > wstrings;

//
//
//
std::wstring::iterator findFirstNonWhitespace
    (
    IN const std::wstring::iterator begin,
    IN const std::wstring::iterator end 
    );

//
// In-place conversion of a string to upper case
//
void toUpper
    (
    IN std::wstring& s
    );

//
//
//
bool lineIsEmpty
    (
    IN std::wstring &line
    );

//
// String compare with optional case sensitivity setting
//
int stringCompare
    (
    IN  const std::wstring& s1,
    IN  const std::wstring& s2,
    OUT bool          caseSensitive
    );

//
// Removes whitechars from both sides of a string
//
std::wstring trim
    (
    IN std::wstring s
    );

//
// Splits a string into an array of strings
//
void split
    (
    IN  std::wstring& text,
    IN  wchar_t       delimiter,
    OUT wstrings&     items
    );


//
// Converts a string into a number
// Throws if conversion not successful
//
double stringToNumber
    (
    IN  std::wstring& text
    );

//
// Converts a string into a number
// Returns true if conversion succeeded
//
bool stringToNumber
    (
    IN  std::wstring& text,
    OUT double&       number
    );

//
//
//
bool textContainsNumber
    (
    IN std::wstring& text
    );

//
// Simple string pattern matching, supports * and ?
//
bool patternMatch
    (
    IN std::wstring pattern,
    IN std::wstring text
    );

//
// Converts a char to a string
//
std::wstring charToStr
    (
    IN wchar_t c
    );

//
// Converts a char array to a string
//
std::wstring charArrToStr
    (
    IN const wchar_t* c
    );

//
// Converts a wide-char string to ANSI
//
std::string wideCharToAnsi
    (
    IN const std::wstring& text
    );

//
//
//
enum class EncodingType
{
    ANSI,
    UTF8,
    UTF16_BigEndian,
    UTF16_LittleEndian,
    UTF32_BigEndian,
    UTF32_LittleEndian
};

//
// Reads the encoding markers from begining of text
// The argument is IN OUT as the method removes the marker, if exists
//
EncodingType getEncodingType
    (
    IN OUT std::wstring& text
    );

//
// Adds the encoding marker to the given text
//
void setEncodingType
    (
    IN     EncodingType  type,
    IN OUT std::wstring& text
    );

//
//
//
bool stringCaseSensitiveLess    (IN std::wstring s1, IN std::wstring s2);
bool stringCaseInsensitiveLess  (IN std::wstring s1, IN std::wstring s2);
bool stringCaseSensitiveEquals  (IN std::wstring s1, IN std::wstring s2);
bool stringCaseInsensitiveEquals(IN std::wstring s1, IN std::wstring s2);
