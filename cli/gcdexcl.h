#pragma once

#include "ccommon.h"
#include "gcdmodel.h"
namespace pcc = pictcli_constraints;

namespace pictcli_gcd
{

class ConstraintsInterpreter
{
public:
    ConstraintsInterpreter( IN CModelData& modelData, IN std::vector< Parameter* >& gcdParameters )
        : _modelData( modelData ), _gcdParameters ( gcdParameters )
    {
        _constrModel.CaseSensitive = _modelData.CaseSensitive;
    }

    bool ConvertToExclusions( CGcdExclusions& gcdExclusions );

    const wstrings& GetWarnings() { return( _warnings ); }

private:
    void interpretConstraint    ( IN const pcc::CConstraint& constraint, IN OUT CGcdExclusions& gcdExclusions );
    void interpretSyntaxTreeItem( IN pcc::CSyntaxTreeItem* item,         IN OUT CGcdExclusions& gcdExclusions );
    void interpretTerm          ( IN pcc::CTerm* term,                   IN OUT CGcdExclusions& gcdExclusions );
    void interpretFunction      ( IN pcc::CFunction* function,           IN OUT CGcdExclusions& gcdExclusions );

    void removeContradictingExclusions( IN OUT CGcdExclusions& gcdExclusions );

    bool isRelationSatisfied       ( IN double diff,         IN pcc::RelationType relationType );
    bool isNumericRelationSatisfied( IN double value,        IN pcc::RelationType relationType, IN double valueToCompareWith );
    bool isStringRelationSatisfied ( IN std::wstring& value, IN pcc::RelationType relationType, IN std::wstring& valueToCompareWith );

    bool valueSatisfiesRelation    ( IN pcc::CParameter& parameter, IN CModelValue& value,
                                     IN pcc::RelationType relationType, IN pcc::CValue* data );

    pcc::DataType getParameterDataType( CModelParameter& parameter );
    std::wstring getConstraintTextForContext( std::wstring& constraintsText, std::wstring::iterator position );

    CModelData&                _modelData;
    std::vector< Parameter* >& _gcdParameters;
    pcc::CModel                _constrModel;
    wstrings                   _warnings;
};

}
