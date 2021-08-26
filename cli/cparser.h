#pragma once

#include <stack>
#include <list>
#include "ccommon.h"
#include "model.h"

namespace pictcli_constraints
{

//
//
//
enum class ValidationErrType
{
    ParameterComparedToValueOfDifferentType,
    ParametersOfDifferentTypesCompared,
    ParameterComparedToItself,
    ParameterValueSetTypeMismatch,
    LIKECannotBeUsedForNumericParameters,
    LIKECannotBeUsedWithNumericValues
};

//
//
//
class CErrValidation
{
public:
    CErrValidation( IN ValidationErrType type, IN unsigned int errInConstraint ):
            Type( type ), ErrInConstraint( errInConstraint ) {}

    CErrValidation( IN ValidationErrType type ):
            Type( type ), ErrInConstraint( 0xFFFFFFFF ) {}

    ValidationErrType Type;
    unsigned int      ErrInConstraint;
};

//
//
//
enum class ValidationWarnType 
{
    UnknownParameter,
};

//
//
//
class CSemanticWarning
{
public:
    CSemanticWarning( IN ValidationWarnType type, IN unsigned int errInConstraint ):
            Type( type ), ErrInConstraint( errInConstraint ) {}

    CSemanticWarning( IN ValidationWarnType type ):
            Type( type ), ErrInConstraint( 0xFFFFFFFF ) {}

    ValidationWarnType Type;
    unsigned int       ErrInConstraint;
};

//
//
//
typedef std::stack<CSyntaxTreeItem*> COperands;
typedef std::stack<LogicalOper>      COperators;
typedef std::list<CSemanticWarning>  CSemanticWarnings;

//
//
//
class ConstraintsParser
{
public:
    ConstraintsParser( IN const CTokenLists& tokenLists ):
            _tokenLists( tokenLists ) {}
   
    ~ConstraintsParser()
    {
        deleteSyntaxTrees();
    }

    void GenerateSyntaxTrees();

    CConstraints      GetConstraints() { return( _constraints ); }
    CSemanticWarnings GetWarnings()    { return( _warnings ); }

private:
    CTokenLists       _tokenLists;
    CConstraints      _constraints;
    CSemanticWarnings _warnings;

    CSyntaxTreeItem* constructSyntaxTreeItem( IN CTokenList::iterator tokenBegin,
                                              IN CTokenList::iterator tokenEnd,
                                              IN bool                 negate );
    void deleteSyntaxTrees();

    CSyntaxTreeItem* processOneLogicalOper ( IN COperators& operators, IN COperands&  operands );
    unsigned int     getLogicalOperPriority( IN LogicalOper logicalOper );

    void removeNOTs();
    void removeBranchNOTs( IN CSyntaxTreeItem* item, IN bool carryOver );

    RelationType getOppositeRelationType( IN RelationType relationType );
    FunctionType getOppositeFunction    ( IN FunctionType functionType );

    void verifyTerm          ( CTerm* term  );
    void verifyFunction      ( CFunction* function );
    void verifyConstraint    ( CConstraint& constraint );
    void verifySyntaxTreeItem( CSyntaxTreeItem* item );
};

}
