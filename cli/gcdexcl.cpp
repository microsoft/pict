#include "ctokenizer.h"
#include "cparser.h"
#include "gcdexcl.h"
using namespace std;
using namespace pictcli_constraints;

namespace pictcli_gcd
{

//
// Determines whether a relation is satisfied
// Doesn't interpret INs and LIKEs, though
//
bool ConstraintsInterpreter::isRelationSatisfied( IN double diff, IN Relation relation )
{
    switch( relation )
    {
    case Relation_EQ: return( diff == 0 ); break;
    case Relation_GE: return( diff >= 0 ); break;
    case Relation_GT: return( diff >  0 ); break;
    case Relation_LE: return( diff <= 0 ); break;
    case Relation_LT: return( diff <  0 ); break;
    case Relation_NE: return( diff != 0 ); break;
    case Relation_IN:
    case Relation_NOT_IN:
    case Relation_LIKE:
    case Relation_NOT_LIKE:
    default:
        assert( false );
        return( 0 );
    }
}

//
//
//
bool ConstraintsInterpreter::isNumericRelationSatisfied( IN double   value,
                                                         IN Relation relation,
                                                         IN double   valueToCompareWith )
{
    double diff = value - valueToCompareWith;
    return( isRelationSatisfied( diff, relation ));
}

//
//
//
bool ConstraintsInterpreter::isStringRelationSatisfied( IN wstring& value,
                                                        IN Relation relation,
                                                        IN wstring& valueToCompareWith )
{
    // take care of LIKEs
    if ( Relation_LIKE     == relation
      || Relation_NOT_LIKE == relation )
    {
        wstring text    = value;
        wstring pattern = valueToCompareWith;
        if ( ! _modelData.CaseSensitive )
        {
            toUpper( text );
            toUpper( pattern );
        }

        bool ret = patternMatch( pattern, text );
        if ( Relation_NOT_LIKE == relation ) 
        {
            ret = ! ret;
        }
        return( ret );
    }
    // and all other relations except INs
    else
    {
        double diff = (double) stringCompare( value, valueToCompareWith, _modelData.CaseSensitive );
        return( isRelationSatisfied( diff, relation ));
    }
}

//
//
//
bool ConstraintsInterpreter::valueSatisfiesRelation( IN CParameter&  parameter,
                                                     IN CModelValue& value,
                                                     IN Relation     relation,
                                                     IN CValue*      data )
{
    wstrings names = value.GetNamesForComparisons();
    for( auto & name : names )
    {
        bool relSatisfied = false;
        if ( parameter.Type == DataType_Number )
        {
            relSatisfied = isNumericRelationSatisfied( stringToNumber( name ), relation, data->Number );
        }
        else if ( parameter.Type == DataType_String )
        {
            relSatisfied = isStringRelationSatisfied( name, relation, data->Text );
        }
        else
        {
            assert( false );
        }

        if ( relSatisfied ) return( true );
    }
    return( false );
}

//
//
//
void ConstraintsInterpreter::interpretTerm( IN CTerm* term, OUT CGcdExclusions& gcdExclusions )
{
    // a useful simplification: gcdData.Parameters, constrModel.Parameters, and modelData.Parameters
    // have elements in the same order thus indexes correspond to the same parameters

    // find the param in modelData
    vector< CModelParameter >::iterator found = _modelData.FindParameterByName( term->Parameter->Name );
    assert( found != _modelData.Parameters.end() );
    CModelParameter& modelParam = *found;

    // find the parameter in gcdParam
    unsigned int paramIdx = (unsigned int) distance( _modelData.Parameters.begin(), found );
    CParameter parameter = _constrModel.Parameters[ paramIdx ];

    // set up the structure: a vector of as many elements as there are values, initially set all
    //  to false; then for every value that satisfies the relation set corresponding element to true
    vector< bool > satisfyingValues;
    satisfyingValues.resize( modelParam.Values.size(), false );

    // do the solving depending on how the term looks like
    switch( term->DataType )
    {
    // everything except INs
    case SyntaxTermDataType_Value:
        {
            for( size_t valueIdx = 0; valueIdx < modelParam.Values.size(); ++valueIdx )
            {
                CModelValue& value = modelParam.Values[ valueIdx ];
                if( valueSatisfiesRelation( parameter, value, term->Relation, (CValue*) term->Data ) )
                {
                    satisfyingValues[ valueIdx ] = true;
                }
            }
            break;
        }
    // INs
    case SyntaxTermDataType_ValueSet:
        {
            assert( term->Relation == Relation_IN 
                 || term->Relation == Relation_NOT_IN );

            for( size_t valueIdx = 0; valueIdx < modelParam.Values.size(); ++valueIdx )
            {
                CModelValue& value = modelParam.Values[ valueIdx ];
            
                // any name must satisfy any value
                bool satisfied = false;
                CValueSet* valueSet = (CValueSet*) term->Data;
                for( auto & vset : *valueSet )
                {
                    if ( valueSatisfiesRelation( parameter, value, Relation_EQ, &vset ))
                    {
                        satisfied = true;
                        break;
                    }
                }

                if (( Relation_IN     == term->Relation &&   satisfied )
                 || ( Relation_NOT_IN == term->Relation && ! satisfied ))
                {
                    satisfyingValues[ valueIdx ] = true;
                }
            }
            break;
        }

    case SyntaxTermDataType_ParameterName:
        {
            // find the param in modelData
            vector< CModelParameter >::iterator found1 = _modelData.FindParameterByName( term->Parameter->Name );
            assert( found1 != _modelData.Parameters.end() );
            CModelParameter& modelParam1 = *found1;
            unsigned int param1Idx = (unsigned int) distance( _modelData.Parameters.begin(), found1 );
            CParameter parameter1 = _constrModel.Parameters[ param1Idx ];

            vector< CModelParameter >::iterator found2 = _modelData.FindParameterByName( ((CParameter*) term->Data)->Name );
            assert( found2 != _modelData.Parameters.end() );
            CModelParameter& modelParam2 = *found2;
            unsigned int param2Idx = (unsigned int) distance( _modelData.Parameters.begin(), found2 );
            CParameter parameter2 = _constrModel.Parameters[ param2Idx ];

            for( unsigned int value1Idx = 0; value1Idx < modelParam1.Values.size(); ++value1Idx )
            {
                CModelValue value1 = modelParam1.Values[ value1Idx ];
                // if any name of value1 satifies the relation with any of the names of value2, values match
                for( unsigned int value2Idx = 0; value2Idx < modelParam2.Values.size(); ++value2Idx )
                {
                    CModelValue value2 = modelParam2.Values[ value2Idx ];

                    for( auto & name2 : value2.GetNamesForComparisons() )
                    {
                        // create temp CValue to use compare function
                        CValue* v2;
                        if ( DataType_Number == parameter2.Type )
                        {
                            v2 = new CValue( stringToNumber( name2 ));
                        }
                        else
                        {
                            v2 = new CValue( name2 );
                        }

                        if( valueSatisfiesRelation( parameter1, value1, term->Relation, v2 ))
                        {
                            Exclusion newExcl;

                            newExcl.insert( make_pair( _gcdParameters[ param1Idx ], value1Idx ));
                            newExcl.insert( make_pair( _gcdParameters[ param2Idx ], value2Idx ));

                            gcdExclusions.insert( newExcl );
                        }
                        delete( v2 );
                    }
                }
            }
        }
    }

    // satisfying values for Value and ValueSet must be now converted into exclusions
    // for ParameterName terms this has been done already
    if ( term->DataType != SyntaxTermDataType_ParameterName )
    {
        // warn if no value or all values satisfy the relation
        bool oneSatifies = false;
        bool allSatisfy  = true;
        for( auto value : satisfyingValues )
        {
            if ( value )
            {
                oneSatifies = true;
            }
            else
            {
                allSatisfy  = false;
            }
        }

        if ( ! oneSatifies || allSatisfy )
        {
            wstring s = L"All or no values satisfy relation ";
            s += (wchar_t*) (term->RawText).c_str();
            _warnings.push_back( s );
        }

        // create exclusions
        for( unsigned int idx = 0; idx < satisfyingValues.size(); ++idx )
        {
            if( satisfyingValues[ idx ] )
            {
                Exclusion newExcl;
                newExcl.insert( make_pair( _gcdParameters[ paramIdx ], idx ));
                gcdExclusions.insert( newExcl );
            }
        }
    }
}

//
//
//
void ConstraintsInterpreter::interpretFunction( IN CFunction* function, IN OUT CGcdExclusions &gcdExclusions )
{
    switch( function->Type )
    {
    case FunctionTypeIsNegativeParam:
    case FunctionTypeIsPositiveParam:
        {
            // a useful simplification: gcdData.Parameters, constrModel.Parameters, and modelData.Parameters
            // have elements in the same order thus indexes correspond to the same parameters
            vector< CModelParameter >::iterator found = _modelData.FindParameterByName( ( static_cast<CParameter*> ( function->Data ))->Name );
            CModelParameter& modelParam = *found;

            // find the parameter in gcdParam
            unsigned int paramIdx = (unsigned int) distance( _modelData.Parameters.begin(), found );

            // add values to the result
            for( unsigned int idx = 0; idx < modelParam.Values.size(); ++idx )
            {
                bool positive = modelParam.Values[ idx ].IsPositive();
                if( function->Type == FunctionTypeIsNegativeParam && ! positive
                 || function->Type == FunctionTypeIsPositiveParam &&   positive )
                {
                    Exclusion newExcl;
                    newExcl.insert( make_pair( _gcdParameters[ paramIdx ], idx ));
                    gcdExclusions.insert( newExcl );
                }
            }
            break;
        }
    default:
        {
            assert( false );
            break;
        }
    }
}

//
//
//
void ConstraintsInterpreter::interpretSyntaxTreeItem( IN CSyntaxTreeItem* item, IN OUT CGcdExclusions& gcdExclusions )
{
    if ( NULL == item ) return;

    // a case where where there is no condition
    if ( ItemType_Term == item->Type )
    {
        interpretTerm( (CTerm*)item->Data, gcdExclusions );
    }
    // a function call to intepret
    else if ( ItemType_Function == item->Type )
    {
        interpretFunction( (CFunction*)item->Data, gcdExclusions );
    }
    // otherwise it is a cross-product or a union
    else if ( ItemType_Node == item->Type )
    {
        CSyntaxTreeNode* node = (CSyntaxTreeNode*) item->Data;

        CGcdExclusions leftExclusions;
        interpretSyntaxTreeItem( node->LLink, leftExclusions );
        CGcdExclusions rightExclusions;
        interpretSyntaxTreeItem( node->RLink, rightExclusions );

        if ( LogicalOper_AND == node->Oper )
        {
            for( auto & left : leftExclusions )
            {
                for( auto & right : rightExclusions )
                {
                    Exclusion newExcl;
                    __insert( newExcl, left.begin(),  left.end() );
                    __insert( newExcl, right.begin(), right.end() );
                    gcdExclusions.insert( newExcl );
                }
            }
        }
        else if ( LogicalOper_OR == node->Oper )
        {
            __insert( gcdExclusions, leftExclusions.begin(),  leftExclusions.end() );
            __insert( gcdExclusions, rightExclusions.begin(), rightExclusions.end() );
        }
        else
        {
            assert( false );
        }
    }
    else
    {
        assert( false );
    }
}

//
//
//
void ConstraintsInterpreter::interpretConstraint( IN const CConstraint& constraint, IN OUT CGcdExclusions& gcdExclusions )
{
    // if there's no condition, look at the term only
    if ( NULL == constraint.Condition )
    {
        interpretSyntaxTreeItem( constraint.Term, gcdExclusions );
    }
    // cross-product for the condition and term otherwise
    else
    {
        CGcdExclusions condExclusions;
        interpretSyntaxTreeItem( constraint.Condition, condExclusions );
    
        CGcdExclusions termExclusions;
        interpretSyntaxTreeItem( constraint.Term, termExclusions);

        for( auto & cond : condExclusions )
        {
            for( auto & term : termExclusions )
            {
                Exclusion newExcl;

                __insert( newExcl, cond.begin(), cond.end() );
                __insert( newExcl, term.begin(), term.end() );

                gcdExclusions.insert( newExcl );
            }
        }
    } 
}

//
// contradicting exclusion refer to the same parameter but have different values
// e.g.: A:1, A:2, B:1
//
void ConstraintsInterpreter::removeContradictingExclusions( IN OUT CGcdExclusions& gcdExclusions )
{
    CGcdExclusions::iterator i_excl = gcdExclusions.begin();
    while ( i_excl != gcdExclusions.end() )
    {
        bool obsolete = false;
        for ( Exclusion::iterator i_ecurr =  i_excl->begin();
                                  i_ecurr != i_excl->end();
                                  i_ecurr++ )
        {
            Exclusion::iterator i_enext = i_ecurr;
            if ( ++i_enext == i_excl->end() ) break;
            if ( i_ecurr->first == i_enext->first ) 
            {
                obsolete = true;
                break;
            }
        }
        if ( obsolete )
        {
            i_excl = __map_erase( gcdExclusions, i_excl );
        }
        else
        {
            i_excl++;
        }
    }
}

//
//
//
DataType ConstraintsInterpreter::getParameterDataType( CModelParameter& parameter )
{
    // assume numeric until proven otherwise
    DataType parameterType = DataType_Number;
    for( auto & value : parameter.Values )
    {
        for( auto & name : value.GetNamesForComparisons() )
        {
            if( !textContainsNumber( name ) )
            {
                parameterType = DataType_String;
                break;
            }
        }
    }
    return( parameterType );
}

//
//
//
wstring ConstraintsInterpreter::getConstraintTextForContext( wstring& constraintsText, wstring::iterator position )
{
    assert( 1 == wcslen( TEXT_TokenConstraintEnd ) );

    // prepare a readable error message
    wstring::iterator i_begin = position;
    while( i_begin != constraintsText.begin() )
    {
        i_begin--;
        if( *i_begin == TEXT_TokenConstraintEnd[ 0 ] )
        {
            i_begin++;
            break;
        }
    }

    wstring::iterator i_end = position;
    while( i_end != constraintsText.end() )
    {
        if( *i_end++ == TEXT_TokenConstraintEnd[ 0 ] ) break;
    }

    wstring contextText;
    contextText.assign( i_begin, i_end );
    return contextText;
}

//
//
//
bool ConstraintsInterpreter::ConvertToExclusions( OUT CGcdExclusions& gcdExclusions )
{
    // translate parameters to a form understandable by the core engine
    for( auto & param : _modelData.Parameters )
    {
        CParameter parameter;
        parameter.Name        = param.Name;
        parameter.Type        = getParameterDataType(param );
        parameter.ResultParam = param.IsResultParameter;

        _constrModel.Parameters.push_back( parameter );
    }

    // handle the constraints, in multiple phases
    // 1. tokenize the constraints
    try
    {
        ConstraintsTokenizer tokenizer( _constrModel, _modelData.ConstraintPredicates );
        try
        {
            tokenizer.Tokenize();
        }
        catch( CSyntaxError e )
        {
            wstring text = getConstraintTextForContext( _modelData.ConstraintPredicates, e.ErrAtPosition );
            wchar_t* failureContext = (wchar_t*) text.c_str();

            // print message
            switch( (SyntaxErrorType) e.Type )
            {
            case SyntaxErrType_UnexpectedEndOfString:
                PrintMessage( InputDataError, L"Constraint ended unexpectedly:", failureContext );
                break;
            case SyntaxErrType_UnknownSpecialChar:
                PrintMessage( InputDataError, L"Non-special character was escaped:", failureContext );
                break;
            case SyntaxErrType_UnknownRelation:
                PrintMessage( InputDataError, L"Missing or incorrect relation:", failureContext );
                break;
            case SyntaxErrType_NoParameterNameOpen:
                PrintMessage( InputDataError, L"Missing opening bracket or misplaced keyword:", failureContext );
                break;
            case SyntaxErrType_NoParameterNameClose:
                PrintMessage( InputDataError, L"Missing closing bracket after parameter name:", failureContext );
                break;
            case SyntaxErrType_NoValueSetOpen:
                PrintMessage( InputDataError, L"Set of values should start with '{':", failureContext );
                break;
            case SyntaxErrType_NoValueSetClose:
                PrintMessage( InputDataError, L"Set of values should end with '}':", failureContext );
                break;
            case SyntaxErrType_NotNumericValue:
                PrintMessage( InputDataError, L"Incorrect numeric value:", failureContext );
                break;
            case SyntaxErrType_NoKeywordThen:
                PrintMessage( InputDataError, L"Misplaced THEN keyword or missing logical operator:", failureContext );
                break;
            case SyntaxErrType_NotAConstraint:
                PrintMessage( InputDataError, L"Constraint definition is incorrect:", failureContext );
                break;
            case SyntaxErrType_NoConstraintEnd:
                PrintMessage( InputDataError, L"Constraint terminated incorectly:", failureContext );
                break;
            case SyntaxErrType_NoEndParenthesis:
                PrintMessage( InputDataError, L"Missing closing parenthesis:", failureContext );
                break;
            case SyntaxErrType_FunctionNoParenthesisOpen:
                PrintMessage( InputDataError, L"Missing opening parenthesis in function:", failureContext );
                break;
            case SyntaxErrType_FunctionNoParenthesisClose:
                PrintMessage( InputDataError, L"Missing closing parenthesis in function:", failureContext );
                break;
            default:
                assert( false );
                break;
            }

            return( false );
        }

        // 2. parse the constraints
        ConstraintsParser parser( tokenizer.GetTokenLists() );

        try
        {
            parser.GenerateSyntaxTrees();

            for( auto & warning : parser.GetWarnings() )
            {
                switch( warning.Type )
                {
                case ValidationWarnType_UnknownParameter:
                    {
                    wstring constraintText = _modelData.GetConstraintText( warning.ErrInConstraint );
                    wchar_t* failureContext = (wchar_t*) ( constraintText.c_str() );
                    PrintMessage( ConstraintsWarning, L"Constraint", failureContext, L"contains unknown parameter. Skipping..." );
                    break;
                    }
                default:
                    {
                    assert( false );
                    break;
                    }
                }
            }
        }
        catch( CErrValidation e )
        {
            wstring constraintText = _modelData.GetConstraintText( e.ErrInConstraint );
            wchar_t* failureContext = (wchar_t*) ( constraintText.c_str() );
            
            switch( e.Type )
            {
            case ValidationErrType_ParameterComparedToValueOfDifferentType:
                PrintMessage( InputDataError, L"Parameter/value type mismatch:", failureContext );
                break;
            case ValidationErrType_ParametersOfDifferentTypesCompared:
                PrintMessage( InputDataError, L"Parameters are of different types:", failureContext );
                break;
            case ValidationErrType_ParameterComparedToItself:
                PrintMessage( InputDataError, L"Parameter cannot be compared to itself:", failureContext );
                break;
            case ValidationErrType_ParameterValueSetTypeMismatch:
                PrintMessage( InputDataError, L"Parameter/value type mismatch:", failureContext );
                break;
            case ValidationErrType_LIKECannotBeUsedForNumericParameters:
                PrintMessage( InputDataError, L"LIKE operator cannot be used for numeric parameters:", failureContext );
                break;
            case ValidationErrType_LIKECannotBeUsedWithNumericValues:
                PrintMessage( InputDataError, L"LIKE operator cannot be used for numeric values:", failureContext );
                break;
            default:
                assert( false );
                break;
            }

            return( false );
        }

        // 3. interpret and translate into a form understanable by the core engine
        const CConstraints& constraints = parser.GetConstraints();

        if ( _modelData.Verbose )
        {
            PrintLogHeader( L"Constraints: Output from syntax parsing" );
            for( auto & c : constraints )
            {
                c.Print();
            }
        }

        for( auto & c : constraints )
        {
            interpretConstraint( c, gcdExclusions );
        }

        // last-minute cleanup
        removeContradictingExclusions( gcdExclusions );
    }
    catch( std::bad_alloc e )
    { 
        throw new GenerationError( __FILE__, __LINE__, OutOfMemory );
    }

    return( true );
}

}
