#pragma once

#include <vector>
#include <list>
#include <cassert>
#include "strings.h"

namespace pictcli_constraints
{

//
//
//
enum class DataType
{
    String,
    Number
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
enum class TokenType
{
    KeywordIf,
    KeywordThen,
    KeywordElse,
    ParenthesisOpen,
    ParenthesisClose,
    LogicalOper,
    Term,
    Function
};

//
//
//
enum class LogicalOper
{
    Or,
    And,
    Not,
    Unknown
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
    CValue( IN std::wstring text ) : DataType( DataType::String ), Text( text ), Number( 0 ) {}
    CValue( IN double number )     : DataType( DataType::Number ), Text( L"" ),  Number( number ) {}

    pictcli_constraints::DataType  DataType;
    std::wstring                   Text;
    double                         Number;
};

typedef std::list<CValue> CValueSet;

//
//
//
enum class TermDataType
{
    ParameterName, // another parameter (every relation except IN and LIKE)
    Value,         // value (every relation except IN)
    ValueSet       // a set of values (IN only)
};

//
//
//
enum class RelationType
{
    Eq,       // =
    Ne,       // <>
    Lt,       // <
    Le,       // <=
    Gt,       // >
    Ge,       // >=

    In,       // IN
    Like,     // LIKE

    NotIn,   // negation of IN
    NotLike, // negation of LIKE

    Unknown
};

//
//
//
class CTerm
{
public:
    CTerm
        (
        IN CParameter*  parameter, // what parameter the term relates to
        IN RelationType relationType,  // what is the relation
        IN TermDataType dataType,  // type of the right side of the relation
        IN void*        data,      // data of the right side of the relation
        IN std::wstring rawText    // raw text of the term, useful for warnings
        ) :
            Parameter    ( parameter ),
            DataType     ( dataType ),
            RelationType ( relationType ),
            RawText      ( rawText ),
            Data         ( data ) {}

    CTerm( CTerm& Term ) :
        Parameter   ( Term.Parameter ),
        DataType    ( Term.DataType ),
        RelationType( Term.RelationType ),
        RawText     ( Term.RawText )
    {
        assert( RelationType < RelationType::Unknown );
        switch( DataType )
        {
        case TermDataType::ParameterName:
        {
            Data = Term.Data;
            break;
        }
        case TermDataType::Value:
        {
            CValue *copyValue = new CValue( *( (CValue*) Term.Data ) );
            Data = copyValue;
            break;
        }
        case TermDataType::ValueSet:
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
        switch( DataType )
        {
        case TermDataType::ParameterName:
            break;
        case TermDataType::Value:
            delete( reinterpret_cast<CValue*>(Data) );
            break;
        case TermDataType::ValueSet:
            delete( reinterpret_cast<CValueSet*>(Data) );
            break;
        default:
            assert(false);
            break;
        }
    }

    CParameter*                       Parameter;
    TermDataType                      DataType;
    pictcli_constraints::RelationType RelationType;
    std::wstring                      RawText;
    void*                             Data;
};

//
//
//
enum class FunctionType
{
    IsNegativeParam, // IsNegative(param)
    IsPositiveParam, // IsPositive(param)
    Unknown
};

//
//
//
enum class FunctionDataType  // function can take parameters
{
    Parameter
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
        // we don't have to delete Data for FunctionDataType::Parameter
        // as it's merely a pointer to an existing data set
        switch( DataType )
        {
        case FunctionDataType::Parameter:
            break;
        default:
            assert(false);
            break;
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
enum class SyntaxErrorType
{
    UnexpectedEndOfString,     // EOS occured when other things were expected
    UnknownSpecialChar,        // Non-special character was escaped in string
    UnknownRelation,           // Relation was expected but none was found
    NoParameterNameOpen,       // Parameter name has no opening marker ([)
    NoParameterNameClose,      // Parameter name has no closing marker (])
    NoValueSetOpen,            // Valueset has no opening marker ({)
    NoValueSetClose,           // Valueset has no closing marker (})
    NotNumericValue,           // Numeric value was expected but none was found
    NoKeywordThen,             // No THEN after IF
    NotAConstraint,            // A string is not in proper constraint format
    NoConstraintEnd,           // No terminating marker encountered (;)
    NoEndParenthesis,          // No closing parenthesis
    FunctionNoParenthesisOpen, // No opening parenthesis on a function
    FunctionNoParenthesisClose // No closing parenthesis on a function
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
        LogicalOper   ( LogicalOper::Unknown ),
        Term          ( nullptr ),
        Function      ( nullptr ) {}

    CToken( IN pictcli_constraints::LogicalOper logicalOper, IN std::wstring::iterator positionInText ) :
        Type          ( TokenType::LogicalOper ),
        PositionInText( positionInText ),
        LogicalOper   ( logicalOper ),
        Term          ( nullptr ),
        Function      ( nullptr ) {}

    CToken( IN CTerm *term, IN std::wstring::iterator positionInText ) :
        Type          ( TokenType::Term ),
        PositionInText( positionInText ),
        LogicalOper   ( LogicalOper::Unknown ),
        Term          ( term ),
        Function      ( nullptr ) {}

    CToken( IN CFunction *function, IN std::wstring::iterator positionInText ) :
        Type          ( TokenType::Function ),
        PositionInText( positionInText ),
        LogicalOper   ( LogicalOper::Unknown ),
        Term          ( nullptr ),
        Function      ( function ) {}

    ~CToken()
    {
        if( nullptr != Term )     delete( Term );
        if( nullptr != Function ) delete( Function );
    }

    TokenType                         Type;
    std::wstring::iterator            PositionInText;
    pictcli_constraints::LogicalOper  LogicalOper;
    CTerm*                            Term;
    CFunction*                        Function;
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
enum class SyntaxTreeItemType
{
    Term,
    Function,
    Node
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
        Type = SyntaxTreeItemType::Term;
        // make a copy of Term so it can be modified freely
        CTerm* copyTerm = new CTerm( *term );
        Data = copyTerm;
    }

    CSyntaxTreeItem( IN CFunction* function )
    {
        Type = SyntaxTreeItemType::Function;
        CFunction* copyFunc = new CFunction( *function );
        Data = copyFunc;
    }

    CSyntaxTreeItem( IN CSyntaxTreeNode* node ) :
        Type( SyntaxTreeItemType::Node ),
        Data( node ) {}

    void Print( unsigned int indent );

    ~CSyntaxTreeItem()
    {
        switch( Type )
        {
        case SyntaxTreeItemType::Term:
            delete( reinterpret_cast<CTerm*>(Data) );
            break;
        case SyntaxTreeItemType::Function:
            delete( reinterpret_cast<CFunction*>(Data) );
            break;
        case SyntaxTreeItemType::Node:
            break;
        default:
            assert(false);
            break;
        }
    }

    SyntaxTreeItemType Type;
    void*              Data = nullptr;
};

//
//
//
class CSyntaxTreeNode
{
public:
    pictcli_constraints::LogicalOper  Oper;
    CSyntaxTreeItem*                  LLink;
    CSyntaxTreeItem*                  RLink;

    CSyntaxTreeNode() : Oper( LogicalOper::Unknown ), LLink( nullptr ), RLink( nullptr ) {}

    ~CSyntaxTreeNode()
    {
        if( nullptr != LLink ) delete( (CSyntaxTreeItem*) LLink );
        if( nullptr != RLink ) delete( RLink );
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
    CConstraint() : Condition( nullptr ), Term( nullptr ) {}

    CSyntaxTreeItem* Condition;
    CSyntaxTreeItem* Term;

    void Print() const;
};

typedef std::vector<CConstraint> CConstraints;

}
