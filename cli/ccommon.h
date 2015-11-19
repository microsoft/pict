#pragma once

#include <vector>
#include <list>
#include <assert.h>
#include "strings.h"

namespace pictcli_constraints
{

//
//
//
enum DataType
{
    DataType_String,
    DataType_Number
};

//
//
//
class CParameter
{
public:
    std::wstring Name;
    DataType     Type;
    bool         ResultParam;
};

typedef std::vector< CParameter > CParameters;

//
//
//
class CModel
{
public:
    CModel() : CaseSensitive( false ) {}

    CParameters Parameters;

    CParameters::iterator findParamByName( IN std::wstring& Name )
    {
        CParameters::iterator found;
        for ( found = Parameters.begin(); found != Parameters.end(); ++found )
        {
            if ( 0 == stringCompare( Name, found->Name, CaseSensitive )) break;
        }
        return( found );
    }

    bool CaseSensitive;
};

//
//
//
enum TokenType
{
    TokenType_KeywordIf,
    TokenType_KeywordThen,
    TokenType_KeywordElse,
    TokenType_ParenthesisOpen,
    TokenType_ParenthesisClose,
    TokenType_LogicalOper,
    TokenType_Term,
    TokenType_Function
};

//
//
//
enum LogicalOper
{
    LogicalOper_OR,
    LogicalOper_AND,
    LogicalOper_NOT,
    LogicalOper_Unknown
};

//
//
//
typedef int LogicalOperPriority;

// BASE must be lower than any other priority
const LogicalOperPriority LogicalOperPriority_BASE = 0;

const LogicalOperPriority LogicalOperPriority_NOT  = 3;
const LogicalOperPriority LogicalOperPriority_AND  = 2;
const LogicalOperPriority LogicalOperPriority_OR   = 1;

//
//
//
class CValue
{
public:
    CValue( IN std::wstring text ) : DataType( DataType_String ), Text( text ) {}
    CValue( IN double number )     : DataType( DataType_Number ), Number( number ) {}

    DataType     DataType;
    std::wstring Text;
    double       Number;
};

typedef std::list<CValue> CValueSet;

//
//
//
enum TermDataType
{
    SyntaxTermDataType_ParameterName, // another parameter (every relation except IN and LIKE)
    SyntaxTermDataType_Value,         // value (every relation except IN)
    SyntaxTermDataType_ValueSet       // a set of values (IN only)
};

//
//
//
enum Relation
{
    Relation_EQ,        // =
    Relation_NE,        // <>
    Relation_LT,        // <
    Relation_LE,        // <=
    Relation_GT,        // >
    Relation_GE,        // >=

    Relation_IN,        // IN
    Relation_LIKE,      // LIKE

    Relation_NOT_IN,    // negation of IN
    Relation_NOT_LIKE,  // negation of LIKE

    Relation_Unknown
};

//
//
//
class CTerm
{
public:
    CTerm
        (
        IN CParameter*   parameter, // what parameter the term relates to
        IN Relation      relation,  // what is the relation
        IN TermDataType  dataType,  // type of the right side of the relation
        IN void*         data,      // data of the right side of the relation
        IN std::wstring  rawText    // raw text of the term, useful for warnings
        ) :
            Parameter( parameter ),
            Relation ( relation ),
            DataType ( dataType ),
            Data     ( data ),
            RawText  ( rawText ) {}

    CTerm( CTerm& Term ) :
        Parameter( Term.Parameter ),
        Relation ( Term.Relation ),
        DataType ( Term.DataType ),
        RawText  ( Term.RawText )
    {
        assert( Relation < Relation_Unknown );
        switch( DataType )
        {
        case SyntaxTermDataType_ParameterName:
        {
            Data = Term.Data;
            break;
        }
        case SyntaxTermDataType_Value:
        {
            CValue *copyValue = new CValue( *( (CValue*) Term.Data ) );
            Data = copyValue;
            break;
        }
        case SyntaxTermDataType_ValueSet:
        {
            CValueSet *copyValue = new CValueSet( *( (CValueSet*) Term.Data ) );
            Data = copyValue;
            break;
        }
        default:
        {
            assert( false );
            break;
        }
        }
    }

    void Print();

    ~CTerm()
    {
        if( SyntaxTermDataType_ParameterName != DataType )
        {
            delete( Data );
        }
    }

    CParameter*   Parameter;
    TermDataType  DataType;
    Relation      Relation;
    void*         Data;
    std::wstring  RawText;
};

//
//
//
enum FunctionType
{
    FunctionTypeIsNegativeParam, // IsNegative(param)
    FunctionTypeIsPositiveParam, // IsPositive(param)
    FunctionTypeUnknown
};

//
//
//
enum FunctionDataType  // function can take parameters
{
    FunctionDataType_Parameter
};

//
//
//
class CFunction
{
public:
    CFunction
        (
        IN FunctionType     type,      // which function
        IN FunctionDataType dataType,  // what we store in Data
        IN void*            data,      // payload
        IN std::wstring     dataText,  // what is the text associated with Data
        IN std::wstring     rawText    // raw text of the term, useful for warnings
        ) :
            Type    ( type ),
            DataType( dataType ),
            Data    ( data ),
            DataText( dataText ),
            RawText ( rawText ) {}

    CFunction
        (
        CFunction& function
        ) :
            Type    ( function.Type ),
            DataType( function.DataType ),
            Data    ( function.Data ),
            DataText( function.DataText ),
            RawText ( function.RawText ) {}

    ~CFunction()
    {
        // we don't have to delete Data for FunctionDataType_Parameter
        // as it's merely a pointer to an existing data set
        if( FunctionDataType_Parameter != DataType )
        {
            delete( Data );
        }
    }

    void Print();

    FunctionType     Type;
    FunctionDataType DataType;
    void*            Data;
    std::wstring     DataText;
    std::wstring     RawText;
};

//
//
//
enum SyntaxErrorType
{
    SyntaxErrType_UnexpectedEndOfString,     // EOS occured when other things were expected
    SyntaxErrType_UnknownSpecialChar,        // Non-special character was escaped in string
    SyntaxErrType_UnknownRelation,           // Relation was expected but none was found
    SyntaxErrType_NoParameterNameOpen,       // Parameter name has no opening marker ([)
    SyntaxErrType_NoParameterNameClose,      // Parameter name has no closing marker (])
    SyntaxErrType_NoValueSetOpen,            // Valueset has no opening marker ({)
    SyntaxErrType_NoValueSetClose,           // Valueset has no closing marker (})
    SyntaxErrType_NotNumericValue,           // Numeric value was expected but none was found
    SyntaxErrType_NoKeywordThen,             // No THEN after IF
    SyntaxErrType_NotAConstraint,            // A string is not in proper constraint format
    SyntaxErrType_NoConstraintEnd,           // No terminating marker encountered (;)
    SyntaxErrType_NoEndParenthesis,          // No closing parenthesis
    SyntaxErrType_FunctionNoParenthesisOpen, // No opening parenthesis on a function
    SyntaxErrType_FunctionNoParenthesisClose // No closing parenthesis on a function
};

//
//
//
class CSyntaxError
{
public:
    CSyntaxError( IN SyntaxErrorType type, IN std::wstring::iterator errAtPosition ) :
        Type( type ), ErrAtPosition( errAtPosition ) {}

    SyntaxErrorType        Type;
    std::wstring::iterator ErrAtPosition;
};

//
//
//
class CToken
{
public:
    CToken( IN TokenType type, IN std::wstring::iterator positionInText ) :
        Type          ( type ),
        PositionInText( positionInText ),
        LogicalOper   ( LogicalOper_Unknown ),
        Term          ( NULL ),
        Function      ( NULL ) {}

    CToken( IN LogicalOper logicalOper, IN std::wstring::iterator positionInText ) :
        Type          ( TokenType_LogicalOper ),
        PositionInText( positionInText ),
        LogicalOper   ( logicalOper ),
        Term          ( NULL ),
        Function      ( NULL ) {}

    CToken( IN CTerm *term, IN std::wstring::iterator positionInText ) :
        Type          ( TokenType_Term ),
        PositionInText( positionInText ),
        LogicalOper   ( LogicalOper_Unknown ),
        Term          ( term ),
        Function      ( NULL ) {}

    CToken( IN CFunction *function, IN std::wstring::iterator positionInText ) :
        Type          ( TokenType_Function ),
        PositionInText( positionInText ),
        LogicalOper   ( LogicalOper_Unknown ),
        Term          ( NULL ),
        Function      ( function ) {}

    ~CToken()
    {
        if( NULL != Term )     delete( Term );
        if( NULL != Function ) delete( Function );
    }

    TokenType              Type;
    std::wstring::iterator PositionInText;
    LogicalOper            LogicalOper;
    CTerm*                 Term;
    CFunction*             Function;
};

//
// Collection of token lists: each item represents one contraint.
// NB: Collection deletes all underlying data so keep it
//     active until combinations are calculated.
//
class CTokenList
{
public:
    typedef std::list<CToken*>::iterator iterator;

    iterator begin()                     { return( _col.begin() ); }
    iterator end()                       { return( _col.end() ); }
    void push_back( CToken* t )          { _col.push_back( t ); }
    void insert( iterator i, CToken* t ) { _col.insert( i, t ); }
    iterator erase( iterator i )         { return( _col.erase( i )); }

    void Print();

private:
    std::list<CToken*> _col;
};

typedef std::list<CTokenList> CTokenLists;

//
//
//
enum SyntaxTreeItemType
{
    ItemType_Term,
    ItemType_Function,
    ItemType_Node
};

//
//
//
class CSyntaxTreeNode;

class CSyntaxTreeItem
{
public:
    CSyntaxTreeItem( IN CTerm* term )
    {
        Type = ItemType_Term;
        // make a copy of Term so it can be modified freely
        CTerm* copyTerm = new CTerm( *term );
        Data = copyTerm;
    }

    CSyntaxTreeItem( IN CFunction* function )
    {
        Type = ItemType_Function;
        CFunction* copyFunc = new CFunction( *function );
        Data = copyFunc;
    }

    CSyntaxTreeItem( IN CSyntaxTreeNode* node ) :
        Type( ItemType_Node ),
        Data( node ) {}

    void Print( unsigned int indent );

    ~CSyntaxTreeItem()
    {
        if( NULL != Data ) delete( Data );
    }

    SyntaxTreeItemType Type;
    void*              Data = NULL;
};

//
//
//
class CSyntaxTreeNode
{
public:
    LogicalOper      Oper;
    CSyntaxTreeItem* LLink;
    CSyntaxTreeItem* RLink;

    CSyntaxTreeNode() : Oper( LogicalOper_Unknown ), LLink( NULL ), RLink( NULL ) {}

    ~CSyntaxTreeNode()
    {
        if( NULL != LLink ) delete( (CSyntaxTreeItem*) LLink );
        if( NULL != RLink ) delete( RLink );
    }
};

//
// Constraint is of two different kinds:
//   condition - term
//   term
// Condition is a syntax tree
//
class CConstraint
{
public:
    CConstraint() : Condition( NULL ), Term( NULL ) {}

    CSyntaxTreeItem* Condition;
    CSyntaxTreeItem* Term;

    void Print() const;
};

typedef std::vector<CConstraint> CConstraints;

}
