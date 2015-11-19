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
    case FunctionTypeIsNegativeParam:
        wcerr << L"IsNegative(";
        break;
    case FunctionTypeIsPositiveParam:
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
    switch( Relation )
    {
    case Relation_EQ:
        wcerr << L"=";
        break;
    case Relation_GE:
        wcerr << L">=";
        break;
    case Relation_GT:
        wcerr << L">";
        break;
    case Relation_IN:
        wcerr << L"in";
        break;
    case Relation_LE:
        wcerr << L"<=";
        break;
    case Relation_LIKE:
        wcerr << L"like";
        break;
    case Relation_LT:
        wcerr << L"<";
        break;
    case Relation_NE:
        wcerr << L"<>";
        break;
    case Relation_NOT_IN:
        wcerr << L"{not in}";
        break;
    case Relation_NOT_LIKE:
        wcerr << L"{not like}";
        break;
    }

    wcerr << L" ";
    switch( DataType )
    {
    case SyntaxTermDataType_ParameterName:
    {
        CParameter* param = (CParameter*) Data;
        wcerr << L"[" << param->Name << L"]";
        break;
    }
    case SyntaxTermDataType_Value:
    {
        CValue* value = (CValue*) Data;
        switch( value->DataType )
        {
        case DataType_Number:
            wcerr << value->Number;
            break;
        case DataType_String:
            wcerr << L"\"" << value->Text << L"\"";
            break;
        }
        break;
    }
    case SyntaxTermDataType_ValueSet:
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
        case TokenType_KeywordIf:
            wcerr << L"IF\n";
            break;
        case TokenType_KeywordThen:
            wcerr << L"THEN\n";
            break;
        case TokenType_ParenthesisOpen:
            wcerr << L"(\n";
            break;
        case TokenType_ParenthesisClose:
            wcerr << L")\n";
            break;
        case TokenType_LogicalOper:
            wcerr << L"LOG: ";
            switch( token->LogicalOper )
            {
            case LogicalOper_AND:
                wcerr << L"AND";
                break;
            case LogicalOper_OR:
                wcerr << L"OR";
                break;
            case LogicalOper_NOT:
                wcerr << L"NOT";
                break;
            default:
                wcerr << L"*** UNKNOWN ***";
                break;
            }
            wcerr << L"\n";
            break;
        case TokenType_Term:
        {
            wcerr << L"TERM ";
            CTerm* term = (CTerm*) token->Term;
            term->Print();
            wcerr << L"\n";
            break;
        }
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
    if( ItemType_Term == Type )
    {
        pindent( indent ); (static_cast<CTerm*>( Data ))->Print();
    }
    else if( ItemType_Function == Type )
    {
        pindent( indent ); (static_cast<CFunction*>( Data ))->Print();
    }
    else
    {
        CSyntaxTreeNode* node = static_cast<CSyntaxTreeNode*>( Data );
        switch( node->Oper )
        {
        case LogicalOper_AND:
            pindent( indent ); wcerr << L"AND\n";
            break;
        case LogicalOper_OR:
            pindent( indent ); wcerr << L"OR\n";
            break;
        case LogicalOper_NOT:
            pindent( indent ); wcerr << L"NOT\n";
            break;
        default:
            pindent( indent ); wcerr << L"*** UNKNOWN ***\n";
            break;
        }

        if( NULL != node->LLink ) node->LLink->Print( indent + 1 );
        if( NULL != node->RLink ) node->RLink->Print( indent + 1 );
    }
}

//
//
//
void CConstraint::Print() const
{
    wcerr << L"Condition:\n";
    if( NULL == Condition )
    {
        wcerr << L" -\n";
    }
    else
    {
        Condition->Print( 1 );
    }

    wcerr << L"Term:\n";
    if( NULL == Term )
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
