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
const wchar_t RESULT_DELIMITER = '\t';

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
    IN wchar_t* text1,
    IN wchar_t* text2 = 0,
    IN wchar_t* text3 = 0
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
