#pragma once

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef NOOP
#define NOOP
#endif

// TODO: move these two somewhere else
#define TEXT_TokenConstraintEnd  L";"
const wchar_t RESULT_DELIMITER = L'\t';

//
// Types of messages
//
enum MsgType
{
    GcdError,
    SystemError,
    InputDataError,
    InputDataWarning,
    ConstraintsWarning,
    RowSeedsError,
    RowSeedsWarning
};

//
// Prints a message to the diagnostic stream
//
void PrintMessage
    (
    IN MsgType  type,
    IN const wchar_t* text1,
    IN const wchar_t* text2 = nullptr,
    IN const wchar_t* text3 = nullptr
    );

//
//
//
void PrintLogHeader
    (
    IN std::wstring title
    );

//
//
//
void PrintStatisticsCaption
    (
    IN const std::wstring& caption
    );
