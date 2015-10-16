// ############################################################################
//
// Arguments handling
//
// ############################################################################

#pragma once

#include "model.h"

const wchar_t PARAMETER_SEPARATOR    = L':';

const wchar_t SWITCH_ORDER           = L'o';
const wchar_t SWITCH_DELIMITER       = L'd';
const wchar_t SWITCH_ALIAS_DELIMITER = L'a';
const wchar_t SWITCH_NEGATIVE_VALUES = L'n';
const wchar_t SWITCH_SEED_FILE       = L'e';
const wchar_t SWITCH_RANDOMIZE       = L'r';
const wchar_t SWITCH_CASE_SENSITIVE  = L'c';
const wchar_t SWITCH_STATISTICS      = L's';
const wchar_t SWITCH_VERBOSE         = L'v';
const wchar_t SWITCH_PREVIEW         = L'p';
const wchar_t SWITCH_APPROXIMATE     = L'x';

//
//
//
bool ParseArgs
(
    IN     int         argc,
    IN     wchar_t*    args[],
    IN OUT CModelData& modelData
);