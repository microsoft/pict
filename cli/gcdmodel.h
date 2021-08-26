#pragma once

#include <sstream>
#include "model.h"

namespace pictcli_gcd
{


//
//
//
enum class ErrorCode
{
    ErrorCode_Success         = 0x00,
    ErrorCode_OutOfMemory     = 0x01,
    ErrorCode_GenerationError = 0x02,
    ErrorCode_BadOption       = 0x03,
    ErrorCode_BadModel        = 0x04,
    ErrorCode_BadConstraints  = 0x05,
    ErrorCode_BadRowSeedFile  = 0x06,
};

//
//
//
class CRow
{
public:
    CRow( wstrings& values, wstrings& decoratedValues, bool negative ) :
        Values( values ), DecoratedValues( decoratedValues ), Negative( negative ) {}

    wstrings Values;
    wstrings DecoratedValues;
    bool     Negative;
};

//
//
//
typedef std::set< Exclusion > CGcdExclusions;

//
//
//
class CResult
{
public:
    std::vector< CRow > TestCases;
    wstrings            SingleItemExclusions;
    wstrings            SolverWarnings;

    void PrintOutput( CModelData& modelData, std::wostream& wout );
    void PrintConstraintWarnings();
    void PrintStatistics();
};

//
//
//
class CGcdData
{
public:
    std::vector< Model* >     Models;
    std::vector< Parameter* > Parameters;
    CGcdExclusions            Exclusions;

    CGcdData( CModelData& modelData ) : _modelData( modelData ) {}

    ~CGcdData()
    {
        // Note: we don't have to clean up Models as the engine itself takes care of that
        // TODO: clean up models, everyone should clean their own structures
        for( auto & i_param : Parameters )
        {
            delete( i_param );
        }
    }

    ErrorCode TranslateToGCD();
    bool FixParamOrder( IN Model* submodel );
    Model* GetRootModel() { return( _task.GetRootModel() ); }

    bool CheckEntireParameterExcluded();
    wstrings GetConstraintWarnings() { return( _constraintWarnings ); }
    wstrings GetSingleItemExclusions();

    void PrintGcdExclusions();

private:
    CModelData& _modelData;
    Task        _task;
    wstrings    _constraintWarnings;

    bool fixModelAndSubmodelOrder();
    void addExclusionsForNegativeRun();
};

}