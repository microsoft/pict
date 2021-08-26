//
// PICT constraints parser
//

#include <valarray>
#include "cparser.h"
using namespace std;

namespace pictcli_constraints
{

//
//
//
void ConstraintsParser::GenerateSyntaxTrees()
{
    // Walk through all tokens. There are three possible types of statements:
    //   1. IF condition THEN term ELSE term2
    //   2. IF condition THEN term
    //   3. term
    for( auto & tokenList : _tokenLists )
    {
        CTokenList::iterator token = tokenList.begin();
        CTokenList::iterator tokenFirst, tokenLast;

        if ( (*token)->Type == TokenType::KeywordIf )
        {
            CConstraint constraintThen;
            CConstraint constraintElse;

            // parse from first item past IF to last before THEN
            tokenFirst = ++token;
            while( TokenType::KeywordThen != (*token)->Type )
            {
                ++token;
            }
            tokenLast = token;
            constraintThen.Condition = constructSyntaxTreeItem( tokenFirst, tokenLast, false );
            constraintElse.Condition = constructSyntaxTreeItem( tokenFirst, tokenLast, true );
            
            // move the pointer past THEN
            tokenFirst = ++tokenLast;
            
            // look for ELSE
            CTokenList::iterator elseKeyword = token;
            while( elseKeyword != tokenList.end() )
            {
                if ( TokenType::KeywordElse == (*elseKeyword)->Type ) break;
                ++elseKeyword;
            }

            // evaluate THEN part; add first constraint to the collection
            constraintThen.Term = constructSyntaxTreeItem( tokenFirst, elseKeyword, true );
            _constraints.push_back( constraintThen );

            // if ELSE part exists, construct the syntax tree, else: clean up
            if ( elseKeyword != tokenList.end() )
            {
                constraintElse.Term = constructSyntaxTreeItem( ++elseKeyword, tokenList.end(), true );
                _constraints.push_back( constraintElse );
            }
            else
            {
                delete( constraintElse.Condition );
            }
        }
        else
        {
            // unconditional; only the Term side exists
            CConstraint constraint;
            constraint.Term = constructSyntaxTreeItem( tokenList.begin(), tokenList.end(), true );
            _constraints.push_back( constraint );
        }
    }

    // we want to get rid of all NOT nodes from the tree to simplify the work
    removeNOTs();

    // validate the constraints
    CConstraints::iterator i_currentConstraint = _constraints.begin();
    unsigned int constraintIndex = 0;
    while( i_currentConstraint != _constraints.end() )
    {
        try
        {
            verifyConstraint( *i_currentConstraint );
            ++i_currentConstraint;
        }
        catch( CSemanticWarning w )
        {
            // add to warnings, remove constraint from collection being returned
            w.ErrInConstraint = constraintIndex;
            _warnings.push_back( w );
            i_currentConstraint = _constraints.erase( i_currentConstraint );
        }
        catch( CErrValidation e )
        {
            // augment with a constraint index and re-throw for the caller to handle
            e.ErrInConstraint = constraintIndex;
            throw e;
        }
        ++constraintIndex;
    }
}

//
// Releases all memory associated with constraints
//
// It's not done in a destructor of CConstraint because the same
//   syntax trees are pointed to from different CConstraint objects and
//   they should persist deletions of any one CConstraint
//
void ConstraintsParser::deleteSyntaxTrees()
{
    for( auto & c : _constraints )
    {
        delete( c.Condition );
        delete( c.Term );
    }
}

//
// Create a CSyntaxTreeItem for a given collection of tokens
//
CSyntaxTreeItem* ConstraintsParser::constructSyntaxTreeItem
    (
    IN CTokenList::iterator tokenBegin,
    IN CTokenList::iterator tokenEnd,
    IN bool                 negate
    )
{
    COperands  operands;
    COperators operators;

    try
    {
        for ( CTokenList::iterator token = tokenBegin; token != tokenEnd; ++token )
        {
            switch( (*token)->Type )
            {

            // pointer to the term is just put onto operands stack.
            case TokenType::Term:
                {
                    CSyntaxTreeItem* item = new CSyntaxTreeItem( ( *token )->Term );
                    operands.push( item );
                    break;
                }

            // pointer to the function goes directly onto operands stack.
            case TokenType::Function:
                {
                    CSyntaxTreeItem* item = new CSyntaxTreeItem( ( *token )->Function );
                    operands.push( item );
                    break;
                }
            
            // logical operator will go directly onto the stack if its priority
            //   is not less than priority of last operator, otherwise process it first.
            case TokenType::LogicalOper:
                {
                    LogicalOper logicalOper = (*token)->LogicalOper;
                    unsigned int currentPriority = getLogicalOperPriority( logicalOper );
                    unsigned int previousPriority;
                    while( true )
                    {
                        if ( operators.empty() )
                        {
                            previousPriority = LogicalOperPriority_BASE;
                        }
                        else
                        {
                            previousPriority = getLogicalOperPriority( operators.top() );
                        }

                        if ( currentPriority >= previousPriority ) break;
                        operands.push( processOneLogicalOper( operators, operands ));
                    }
                    operators.push( logicalOper );
                    break;
                }
            
                
            // for open parethesis, evaluate the contants recursively
            case TokenType::ParenthesisOpen:
                {
                    ++token;
                    tokenBegin = token;

                    // Find matching closing parenthesis
                    unsigned int parenthesesCount = 0;
                    while( parenthesesCount != 0 || (*token)->Type != TokenType::ParenthesisClose )
                    {
                        switch ( (*token)->Type )
                        {
                        case TokenType::ParenthesisOpen:
                            ++parenthesesCount;
                            break;
                        case TokenType::ParenthesisClose:
                            --parenthesesCount;
                            break;
                        default:
                            break;
                        }

                        ++token;
                    }

                    // The outcome of recursive call must go onto the stack.
                    operands.push( constructSyntaxTreeItem( tokenBegin, token, false ));
                    break;
                }
                default:
                    break;
            }
        }

        // If there are any outstanding operators, process them
        // If there are any outstanding operands, the condition didn't have any logical operators in it, only one Term.
        //
        if ( ! operators.empty() )
        {
            while( ! operators.empty() )
            {
                operands.push( processOneLogicalOper( operators, operands )); 
            }
        }

        // After all the processing there must be only one item left on the operands stack
        assert( 1 == operands.size() );

        CSyntaxTreeItem *ret;

        // add a top-most NOT if needed
        if ( negate )
        {
            CSyntaxTreeNode* node = new CSyntaxTreeNode();

            node->Oper  = LogicalOper::Not;
            node->LLink = operands.top();

            CSyntaxTreeItem* item = new CSyntaxTreeItem( node );
            ret = item;
        }
        else
        {
            ret = operands.top();
        }

        return( ret );
    }
    
    // If any error arises we must delete all tree items we put on the stack.
    catch ( ... )
    {
        while ( ! operands.empty() )
        {
            delete( (CSyntaxTreeItem*) operands.top() );
            operands.pop();
        }
        return nullptr;
    }
}

//
// Takes one operator off the stack and creates a tree item with
//   appropriate number of operands
//
CSyntaxTreeItem* ConstraintsParser::processOneLogicalOper( IN COperators& operators,
                                                           IN COperands&  operands )
{
    CSyntaxTreeNode* node = new CSyntaxTreeNode();

    node->Oper = operators.top();
    operators.pop();

    switch( node->Oper )
    {
    case LogicalOper::And:
    case LogicalOper::Or:
        node->RLink = operands.top();
        operands.pop();
        node->LLink = operands.top();
        operands.pop();
        break;
    case LogicalOper::Not:
        node->LLink = operands.top();
        // the right node remains "nullptr"
        operands.pop();
        break;
    default:
        assert( false ); // Undefined logical operator
    }

    CSyntaxTreeItem* item;
    try
    {
        item = new CSyntaxTreeItem( node );
    }
    catch( ... )
    {
        delete( node );
        throw;
    }

    return( item );
}

//
// Returns a priority of logical operator.
//
unsigned int ConstraintsParser::getLogicalOperPriority( IN LogicalOper logicalOper )
{
    switch( logicalOper )
    {
    case LogicalOper::Or:  return( LogicalOperPriority_OR  );
    case LogicalOper::And: return( LogicalOperPriority_AND );
    case LogicalOper::Not: return( LogicalOperPriority_NOT );
    default:
        assert( false );
        return( LogicalOperPriority_BASE );
    }
}

//
// NOTs make generating combinations hard. Remove using the De Morgan laws
// 1. ~(A or  B) == ~A and ~B
// 2. ~(A and B) == ~A or  ~B
//
void ConstraintsParser::removeNOTs()
{
    for( auto & c : _constraints )
    {
        removeBranchNOTs( c.Condition, false );
        removeBranchNOTs( c.Term,      false );
    }
}

//
// Removes a NOT from a branch. The second parameter determines if negation
//   should be imposed on a current branch (carried over from the upper 
//   branch).
//
void ConstraintsParser::removeBranchNOTs( IN CSyntaxTreeItem* item, IN bool carryOver )
{
    if ( nullptr == item) return;

    switch( item->Type )
    {

    // For a term or a function, flip the relation if negation is carried over
    case SyntaxTreeItemType::Term:
        {
            if( carryOver )
            {
                CTerm* term = (CTerm*) item->Data;
                term->RelationType = getOppositeRelationType( term->RelationType );
            }
        }
        break;
    case SyntaxTreeItemType::Function:
        {
            if ( carryOver )
            {
                CFunction* func = (CFunction*) item->Data;
                func->Type = getOppositeFunction( func->Type );
            }
        }
        break;
    
    // What to do with node depends on a logical operator:
    // 1. AND or OR will be changed swapped and the underlying branches will get negated
    // 2. NOT will disappear and the branch will get negated
    case SyntaxTreeItemType::Node:
        {
            CSyntaxTreeNode* node = (CSyntaxTreeNode*) item->Data;
            switch( node->Oper )
            {
            case LogicalOper::And:
                if ( carryOver )
                {
                    node->Oper = LogicalOper::Or;
                }
                removeBranchNOTs( node->LLink, carryOver );
                removeBranchNOTs( node->RLink, carryOver );
                break;
            case LogicalOper::Or:
                if ( carryOver )
                {
                    node->Oper = LogicalOper::And;
                }
                removeBranchNOTs( node->LLink, carryOver );
                removeBranchNOTs( node->RLink, carryOver );
                break;
            case LogicalOper::Not:
                removeBranchNOTs( node->LLink, ! carryOver );
                item->Type = node->LLink->Type;
                item->Data = node->LLink->Data;

                // zero out the data pointer of LLink so during clean-up we don't
                // delete the data that was just assigned to current item
                node->LLink->Data = nullptr;
                delete( node );
                break;
            default:
                assert( false );
            }
        }
        break;
    default:
        assert( false );
    }
}

//
//
//
RelationType ConstraintsParser::getOppositeRelationType( IN RelationType relationType )
{
    RelationType newRelationType = RelationType::Unknown;

    switch( relationType )
    {
    case RelationType::Eq:      newRelationType = RelationType::Ne;      break;
    case RelationType::Ne:      newRelationType = RelationType::Eq;      break;
    case RelationType::Lt:      newRelationType = RelationType::Ge;      break;
    case RelationType::Le:      newRelationType = RelationType::Gt;      break;
    case RelationType::Gt:      newRelationType = RelationType::Le;      break;
    case RelationType::Ge:      newRelationType = RelationType::Lt;      break;
    case RelationType::In:      newRelationType = RelationType::NotIn;   break;
    case RelationType::NotIn:   newRelationType = RelationType::In;      break;
    case RelationType::Like:    newRelationType = RelationType::NotLike; break;
    case RelationType::NotLike: newRelationType = RelationType::Like;    break;
    default: assert( false );
    }

    return(newRelationType);
}

//
//
//
FunctionType ConstraintsParser::getOppositeFunction( IN FunctionType functionType )
{
    FunctionType newFunctionType = FunctionType::Unknown;
    
    switch( functionType )
    {
    case FunctionType::IsNegativeParam: newFunctionType = FunctionType::IsPositiveParam; break;
    case FunctionType::IsPositiveParam: newFunctionType = FunctionType::IsNegativeParam; break;
    default: assert( false ); break;
    }

    return( newFunctionType );
}

//
//
//
void ConstraintsParser::verifyConstraint( CConstraint& constraint )
{
    verifySyntaxTreeItem( constraint.Condition );
    verifySyntaxTreeItem( constraint.Term );
}

//
//
//
void ConstraintsParser::verifySyntaxTreeItem( CSyntaxTreeItem* item )
{
    if ( nullptr == item ) return;

    if ( SyntaxTreeItemType::Term == item->Type )
    {
        verifyTerm( (CTerm*) item->Data );
    }
    else if ( SyntaxTreeItemType::Function == item->Type )
    {
        verifyFunction( (CFunction*) item->Data );
    }
    else
    {
        CSyntaxTreeNode* node = (CSyntaxTreeNode*) item->Data;

        verifySyntaxTreeItem( node->LLink );
        verifySyntaxTreeItem( node->RLink );
    }
}

//
//
//
void ConstraintsParser::verifyTerm( CTerm* term )
{
    // Is Parameter defined in the model?
    if ( term->Parameter == nullptr )
    {
        throw CSemanticWarning( ValidationWarnType::UnknownParameter );
    }
    
    // LIKE is only for string parameters and should have a string value
    if ( term->RelationType == RelationType::Like
      || term->RelationType == RelationType::NotLike )
    {
        if ( term->Parameter->Type == DataType::Number )
        {
            throw CErrValidation( ValidationErrType::LIKECannotBeUsedForNumericParameters );
        }
        
        if ( term->DataType == TermDataType::Value
        && ( (CValue*) term->Data )->DataType == DataType::Number )
        {
            throw CErrValidation( ValidationErrType::LIKECannotBeUsedWithNumericValues );
        }
    }

    // Parameter can only be compared to a value of the same type
    if ( term->DataType == TermDataType::Value )
    {
        if ( term->Parameter->Type != ((CValue*) term->Data )->DataType )
        {
            throw CErrValidation( ValidationErrType::ParameterComparedToValueOfDifferentType );
        }    
    }

    // Is second parameter defined?
    if ( term->DataType == TermDataType::ParameterName )
    {
        if ( term->Data == nullptr )
        {
            throw CSemanticWarning( ValidationWarnType::UnknownParameter );
        }
    }

    // 1. Two parameters to be comparable must be of the same type
    // 2. A parameter shouldn't be compared to itself
    if ( term->DataType == TermDataType::ParameterName )
    {
        if ( term->Parameter->Type != ((CParameter*) term->Data )->Type )
        {
            throw CErrValidation( ValidationErrType::ParametersOfDifferentTypesCompared );
        }    

        if ( term->Parameter->Name == ((CParameter*) term->Data )->Name )
        {
            throw CErrValidation( ValidationErrType::ParameterComparedToItself );
        }
    }

    // IN has a value set on the right side; all values should be of the type of the bound parameter
    if ( term->DataType == TermDataType::ValueSet )
    {
        CValueSet* valueSet = (CValueSet*) term->Data;
        for ( CValueSet::iterator i_val =  valueSet->begin();
                                  i_val != valueSet->end();
                                ++i_val )
        {
            if ( term->Parameter->Type != i_val->DataType )
            {
                throw CErrValidation( ValidationErrType::ParameterValueSetTypeMismatch );
            }
        }
    }
}

//
//
//
void ConstraintsParser::verifyFunction( CFunction *function )
{
    switch( function->Type )
    {
    // functions must have a parameter to operate on
    case FunctionType::IsNegativeParam:
    case FunctionType::IsPositiveParam:
        {
            if ( function->Data == nullptr && ! function->DataText.empty() )
            {
                throw CSemanticWarning( ValidationWarnType::UnknownParameter );
            }
            break;
        }
    default:
        break;
    }
}

}
