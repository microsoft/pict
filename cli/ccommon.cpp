#include <iostream>
#include "ccommon.h"
using namespace std;

namespace pictcli_constraints
{

//
//
//
void CFunction::Print()
{
    switch( Type )
    {
    case FunctionType::IsNegativeParam:
        wcerr << L"IsNegative(";
        break;
    case FunctionType::IsPositiveParam:
        wcerr << L"IsPositive(";
        break;
    default:
        assert( false );
        break;
    }
    wcerr << ( static_cast<CParameter*>( Data ))->Name;
    wcerr << L")\n";
}

//
//
//
void CTerm::Print()
{
    wcerr << L"[" << Parameter->Name << L"] ";
    switch( RelationType )
    {
    case RelationType::Eq:
        wcerr << L"=";
        break;
    case RelationType::Ge:
        wcerr << L">=";
        break;
    case RelationType::Gt:
        wcerr << L">";
        break;
    case RelationType::In:
        wcerr << L"in";
        break;
    case RelationType::Le:
        wcerr << L"<=";
        break;
    case RelationType::Like:
        wcerr << L"like";
        break;
    case RelationType::Lt:
        wcerr << L"<";
        break;
    case RelationType::Ne:
        wcerr << L"<>";
        break;
    case RelationType::NotIn:
        wcerr << L"{not in}";
        break;
    case RelationType::NotLike:
        wcerr << L"{not like}";
        break;
    default:
        break;
    }

    wcerr << L" ";
    switch( DataType )
    {
    case TermDataType::ParameterName:
    {
        CParameter* param = (CParameter*) Data;
        wcerr << L"[" << param->Name << L"]";
        break;
    }
    case TermDataType::Value:
    {
        CValue* value = (CValue*) Data;
        switch( value->DataType )
        {
        case DataType::Number:
            wcerr << value->Number;
            break;
        case DataType::String:
            wcerr << L"\"" << value->Text << L"\"";
            break;
        }
        break;
    }
    case TermDataType::ValueSet:
        wcerr << L"{...}";
        break;
    }
    wcerr << L"\n";
}

//
//
//
void CTokenList::Print()
{
    for( auto & token : _col )
    {
        switch( token->Type )
        {
        case TokenType::KeywordIf:
            wcerr << L"IF\n";
            break;
        case TokenType::KeywordThen:
            wcerr << L"THEN\n";
            break;
        case TokenType::ParenthesisOpen:
            wcerr << L"(\n";
            break;
        case TokenType::ParenthesisClose:
            wcerr << L")\n";
            break;
        case TokenType::LogicalOper:
            wcerr << L"LOG: ";
            switch( token->LogicalOper )
            {
            case LogicalOper::And:
                wcerr << L"AND";
                break;
            case LogicalOper::Or:
                wcerr << L"OR";
                break;
            case LogicalOper::Not:
                wcerr << L"NOT";
                break;
            default:
                wcerr << L"*** UNKNOWN ***";
                break;
            }
            wcerr << L"\n";
            break;
        case TokenType::Term:
        {
            wcerr << L"TERM ";
            CTerm* term = (CTerm*) token->Term;
            term->Print();
            wcerr << L"\n";
            break;
        }
        default:
            break;
        }
    }
}

//
//
//
void pindent( unsigned int indent )
{
    for( unsigned int i = indent; i != 0; i-- )
    {
        wcerr << L" ";
    }
}

//
//
//
void CSyntaxTreeItem::Print( unsigned int indent )
{
    if( SyntaxTreeItemType::Term == Type )
    {
        pindent( indent ); (static_cast<CTerm*>( Data ))->Print();
    }
    else if( SyntaxTreeItemType::Function == Type )
    {
        pindent( indent ); (static_cast<CFunction*>( Data ))->Print();
    }
    else
    {
        CSyntaxTreeNode* node = static_cast<CSyntaxTreeNode*>( Data );
        switch( node->Oper )
        {
        case LogicalOper::And:
            pindent( indent ); wcerr << L"AND\n";
            break;
        case LogicalOper::Or:
            pindent( indent ); wcerr << L"OR\n";
            break;
        case LogicalOper::Not:
            pindent( indent ); wcerr << L"NOT\n";
            break;
        default:
            pindent( indent ); wcerr << L"*** UNKNOWN ***\n";
            break;
        }

        if( nullptr != node->LLink ) node->LLink->Print( indent + 1 );
        if( nullptr != node->RLink ) node->RLink->Print( indent + 1 );
    }
}

//
//
//
void CConstraint::Print() const
{
    wcerr << L"Condition:\n";
    if( nullptr == Condition )
    {
        wcerr << L" -\n";
    }
    else
    {
        Condition->Print( 1 );
    }

    wcerr << L"Term:\n";
    if( nullptr == Term )
    {
        wcerr << L" -\n";
    }
    else
    {
        Term->Print( 1 );
        wcerr << L"\n";
    }
}

}
