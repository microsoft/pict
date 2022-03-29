#include <valarray>
#include <sstream>
#include "ctokenizer.h"
using namespace std;

namespace pictcli_constraints
{

//
// handled by parseConstraint()
//
#define TEXT_TokenKeywordIf             L"IF"  
#define TEXT_TokenKeywordThen           L"THEN"
#define TEXT_TokenKeywordElse           L"ELSE"

//
// handled by getValueSet()
//
#define TEXT_TokenValueSetOpen          L"{"
#define TEXT_TokenValueSetSeparator     L","
#define TEXT_TokenValueSetClose         L"}"

//
// handled by getParameterName()
//
// defined in cp.h as it's used by csolver.cpp
#define TEXT_TokenParameterNameOpen     L"["
#define TEXT_TokenParameterNameClose    L"]"

//
// handled by parseCondition() and getFunction()
//
#define TEXT_TokenParenthesisOpen       L"("
#define TEXT_TokenParenthesisClose      L")"

//
// handled by getFunction()
//
#define TEXT_FunctionIsNegativeParam    L"ISNEGATIVE"
#define TEXT_FunctionIsPositiveParam    L"ISPOSITIVE"

//
// handled by parseTerm()
//
#define TEXT_TokenQuotes                L"\""

//
// handled by getRelation()
//
#define TEXT_TokenRelationEQ            L"=" 
#define TEXT_TokenRelationNE            L"<>"
#define TEXT_TokenRelationLT            L"<" 
#define TEXT_TokenRelationLE            L"<="
#define TEXT_TokenRelationGT            L">" 
#define TEXT_TokenRelationGE            L">="
#define TEXT_TokenRelationIN            L"IN"
#define TEXT_TokenRelationLIKE          L"LIKE"

//
// handled by getLogicalOper()
//
#define TEXT_TokenLogicalOperAND        L"AND"
#define TEXT_TokenLogicalOperOR         L"OR" 

//
// not handled by any function because of grammar; used directly
//
#define TEXT_TokenLogicalOperNOT        L"NOT"

//
// Special characters recognized within a string
//
#define TEXT_SpecialCharMarker          L'\\'

//
// create an array of special characters, then populate valarray with it
//
const wchar_t SpecialCharacters[] = { TEXT_SpecialCharMarker, L'"', L']' };

//
//
//
void ConstraintsTokenizer::Tokenize()
{
    _tokenLists.clear();

    while( _currentPosition < _constraintsText.end() )
    {
        CTokenList tokenList;
        parseConstraint( tokenList );
        _tokenLists.push_back( tokenList );

        skipWhiteChars();
    }
}

//
//
//
void ConstraintsTokenizer::cleanUpTokenLists()
{
    for( auto & tokenList : _tokenLists )
        for( auto & token : tokenList )
            delete( token );
}


// 
// Parses a constraint:
// 
// constraint   ::= IF <clause> THEN <term>;
//                  IF <clause> THEN <term> ELSE <term>;
//                  <parameter_name> <relation> <parameter_name>;
//
void ConstraintsTokenizer::parseConstraint( IN OUT CTokenList& tokens )
{
    skipWhiteChars();

    // save position in case a new token is created
    wstring::iterator position = _currentPosition;
    
    // IF <clause> THEN <clause> ELSE <clause>
    // <clause>
    if ( isNextSubstring( wstring(TEXT_TokenKeywordIf)) )
    {
        CToken* tokenKeywordIf = new CToken( TokenType::KeywordIf, position );
        tokens.push_back( tokenKeywordIf );

        skipWhiteChars();
        parseClause( tokens );
        
        skipWhiteChars();
        position = _currentPosition;
        if ( isNextSubstring( charArrToStr( TEXT_TokenKeywordThen )))
        {
            CToken* tokenKeywordThen = new CToken( TokenType::KeywordThen, position );
            tokens.push_back( tokenKeywordThen );
        }
        else
        {
            throw CSyntaxError( SyntaxErrorType::NoKeywordThen, _currentPosition );
        }
    }

    // evaluate the THEN part
    parseClause( tokens );

    // evaluate the ELSE part
    skipWhiteChars();
    position = _currentPosition;
    if ( isNextSubstring( charArrToStr( TEXT_TokenKeywordElse )))
    {
        CToken* tokenKeywordElse = new CToken( TokenType::KeywordElse, position );
        tokens.push_back( tokenKeywordElse );

        parseClause( tokens );
    }

    // all forms of contraints should end with a termination marker
    skipWhiteChars();
    position = _currentPosition;
    if ( ! isNextSubstring ( charArrToStr( TEXT_TokenConstraintEnd )))
    {
        throw CSyntaxError( SyntaxErrorType::NoConstraintEnd, _currentPosition );
    }

    // some functions are like macros so do the expansions on the token list
    doPostParseExpansions( tokens );
}

// 
// Parses a clause:
// 
// clause       ::= <condition>
//                  <condition> <logical_operator> <clause>
//
void ConstraintsTokenizer::parseClause( IN OUT CTokenList& tokens )
{
    skipWhiteChars();
    parseCondition( tokens );
    
    // getLogicalOper() may change the current position so preserve it for token creation
    skipWhiteChars();
    wstring::iterator position = _currentPosition;
    
    LogicalOper logicalOper = getLogicalOper();
    if ( LogicalOper::Unknown != logicalOper )
    {
        CToken* token = new CToken( logicalOper, position );
        tokens.push_back( token );
        
        skipWhiteChars();
        parseClause( tokens );
    }
}

// 
// Parses a condition:
// 
// condition    ::= <term>
//                  (<clause>)
//                  NOT <clause>
//
void ConstraintsTokenizer::parseCondition( IN OUT CTokenList& tokens )
{
    skipWhiteChars();
    wstring::iterator position = _currentPosition;
    
    // (<clause>)
    if ( isNextSubstring( charArrToStr( TEXT_TokenParenthesisOpen )))
    {
        CToken* token = new CToken( TokenType::ParenthesisOpen, position );;
        tokens.push_back( token );

        skipWhiteChars();
        parseClause( tokens );

        skipWhiteChars();
        position = _currentPosition;
        if ( isNextSubstring( charArrToStr( TEXT_TokenParenthesisClose )))
        {
            token = new CToken( TokenType::ParenthesisClose, position );
            tokens.push_back( token );
        }
        else
        {
            throw CSyntaxError( SyntaxErrorType::NoEndParenthesis, _currentPosition );
        }
    }

    // NOT <clause> 
    else if ( isNextSubstring( charArrToStr( TEXT_TokenLogicalOperNOT )))
    {
        CToken* token = new CToken( LogicalOper::Not, position );
        tokens.push_back( token );
        
        skipWhiteChars();
        parseClause( tokens );
    }

    // <term>
    else
    {
        parseTerm( tokens );
    }
}

// 
// Parses a term:
//
// term         ::= <parameter_name> <relation> <value>
//                  <parameter_name> LIKE <string>
//                  <parameter_name> IN {<value_set>}
//                  <parameter_name> <relation> <parameter_name>
//                  {functions on term level}
//
void ConstraintsTokenizer::parseTerm( IN OUT CTokenList& tokens )
{
    skipWhiteChars();
    wstring::iterator position = _currentPosition;

    // check whether it's one of the functions
    CFunction *function = getFunction();
    if( nullptr != function )
    {
        CToken* token;
        try
        {
            token = new CToken( function, position );
        }
        catch( ... )
        {
            delete( function );
            throw; 
        }
        tokens.push_back( token );
    }
    
    // if not, parse anything that starts with para_name
    else
    {
        wstring paramName = getParameterName();
        CParameters::iterator found = _model.findParamByName( paramName );
        
        CParameter* param = nullptr;
        if ( found != _model.Parameters.end() )
        {
            param = &*found;
        }

        skipWhiteChars();
        RelationType relationType = getRelationType();

        skipWhiteChars();

        CTerm* term = nullptr;
        switch( relationType )
        {
            case RelationType::In:
            case RelationType::NotIn:
            {
                CValueSet* valueSet = new CValueSet;

                if ( ! isNextSubstring( charArrToStr( TEXT_TokenValueSetOpen )))
                {
                    throw CSyntaxError( SyntaxErrorType::NoValueSetOpen, _currentPosition );
                }
                
                try
                {
                    getValueSet( *valueSet );
                }
                catch( ... )
                {
                    delete( valueSet );
                    throw;
                }

                skipWhiteChars();
                if ( ! isNextSubstring( charArrToStr( TEXT_TokenValueSetClose )))
                {
                    throw CSyntaxError( SyntaxErrorType::NoValueSetClose, _currentPosition );
                }

                // raw text of a term
                wstring rawText;
                rawText.assign( position, _currentPosition );

                try
                {
                    term = new CTerm( param, relationType, TermDataType::ValueSet, valueSet, rawText );
                }
                catch( ... )
                {
                    delete( valueSet );
                    throw;
                }
                break;
            }

            // At this point the relation LIKE is treated as an ordinary relation 
            //   despite the fact it can only have a string as an argument on
            //   the right-side. It will be verified later during parsing.
            default:
            {
                if ( isNextSubstring( charArrToStr( TEXT_TokenParameterNameOpen ), true ))
                {
                    wstring paramName2 = getParameterName();
                    
                    //
                    // look up parameters by their names and return references
                    //
                    CParameter *param2 = nullptr;
                    found = _model.findParamByName( paramName2 );
                    if ( found != _model.Parameters.end() )
                    {
                        param2 = &*found;
                    }

                    wstring rawText;
                    rawText.assign( position, _currentPosition );

                    term = new CTerm( param, relationType, TermDataType::ParameterName, param2, rawText );
                }
                else
                {
                    CValue* value = getValue();

                    // raw text of a term
                    wstring rawText;
                    rawText.assign( position, _currentPosition );

                    try
                    {
                        term = new CTerm( param, relationType, TermDataType::Value, value, rawText );
                    }
                    catch( ... )
                    {
                        delete( value );
                        throw;
                    }
                }
                break;
            }
        }

        // now create token of type 'term'; this token has data
        CToken* token;
        try
        {
            token = new CToken( term, position );
        }
        catch( ... )
        {
            delete( term );
            throw; 
        }
        tokens.push_back( token );
    }
}

// 
// Parses a function
//
// <term> ::= IsNegative(<parameter_name>)
//
// Returns a CFunction object if in fact a function was parsed
// or "nullptr" otherwise
//
CFunction *ConstraintsTokenizer::getFunction()
{
    skipWhiteChars();
    wstring::iterator position = _currentPosition;

    FunctionType type = FunctionType::Unknown;

    if ( isNextSubstring( charArrToStr( TEXT_FunctionIsNegativeParam )))
    {
        type = FunctionType::IsNegativeParam;
    }
    else if ( isNextSubstring( charArrToStr( TEXT_FunctionIsPositiveParam )))
    {
        type = FunctionType::IsPositiveParam;
    }
    else
    {
        return nullptr;
    }

    // opening bracket
    if ( ! isNextSubstring( charArrToStr( TEXT_TokenParenthesisOpen )))
    {
        throw CSyntaxError( SyntaxErrorType::FunctionNoParenthesisOpen, _currentPosition );
    }

    // get the parameter name
    skipWhiteChars();
    wstring paramName = getString( charArrToStr( TEXT_TokenParenthesisClose ));
    CParameters::iterator found = _model.findParamByName( paramName );

    CParameter* param = nullptr;
    if ( found != _model.Parameters.end() )
    {
        param = &*found;
    }

    if ( ! isNextSubstring( charArrToStr( TEXT_TokenParenthesisClose )))
    {
        throw CSyntaxError( SyntaxErrorType::FunctionNoParenthesisClose, _currentPosition );
    }

    // now create a CFunction and return it
    wstring rawText;
    rawText.assign( position, _currentPosition );

    CFunction* function = new CFunction( type, FunctionDataType::Parameter, param, paramName, rawText );

    return( function );
}

//
// Returns a CValue.
//
// Note: allocates memory, caller is supposed to free it
//
CValue* ConstraintsTokenizer::getValue()
{
    CValue* value;

    // value is either string or number,
    // a string always begins with quotes so check for it first
    if ( isNextSubstring( charArrToStr( TEXT_TokenQuotes )))
    {
        wstring text;
        text = getString( charArrToStr( TEXT_TokenQuotes ));
        if (! isNextSubstring( charArrToStr( TEXT_TokenQuotes )))
        {
            throw CSyntaxError( SyntaxErrorType::UnexpectedEndOfString, _currentPosition );
        }

        value = new CValue( text );
    }
    else
    {
        double number = getNumber();
        value = new CValue( number );
    }

    return( value );
}

// 
// Parses a valueset
// 
// value_set        ::= <value>
//                      <value>,<value_set>
//
void ConstraintsTokenizer::getValueSet( OUT CValueSet& valueSet )
{
    skipWhiteChars();

    CValue* value = getValue();
    valueSet.push_back( *value );
    delete( value );

    skipWhiteChars();
    if ( isNextSubstring( charArrToStr( TEXT_TokenValueSetSeparator )))
    {
        skipWhiteChars();
        getValueSet( valueSet );
    }
}

//
// Returns the next relation; order of comparisons is important
//
RelationType ConstraintsTokenizer::getRelationType()
{
    if     ( isNextSubstring( charArrToStr( TEXT_TokenRelationEQ     ))) return ( RelationType::Eq   );
    else if( isNextSubstring( charArrToStr( TEXT_TokenRelationNE     ))) return ( RelationType::Ne   );
    else if( isNextSubstring( charArrToStr( TEXT_TokenRelationLE     ))) return ( RelationType::Le   );
    else if( isNextSubstring( charArrToStr( TEXT_TokenRelationGE     ))) return ( RelationType::Ge   );
    else if( isNextSubstring( charArrToStr( TEXT_TokenRelationGT     ))) return ( RelationType::Gt   );
    else if( isNextSubstring( charArrToStr( TEXT_TokenRelationLT     ))) return ( RelationType::Lt   );
    else if( isNextSubstring( charArrToStr( TEXT_TokenRelationIN     ))) return ( RelationType::In   );
    else if( isNextSubstring( charArrToStr( TEXT_TokenRelationLIKE   ))) return ( RelationType::Like );
    else if( isNextSubstring( charArrToStr( TEXT_TokenLogicalOperNOT )))
    {
        skipWhiteChars();
        if     ( isNextSubstring( charArrToStr( TEXT_TokenRelationIN   ))) return ( RelationType::NotIn   );
        else if( isNextSubstring( charArrToStr( TEXT_TokenRelationLIKE ))) return ( RelationType::NotLike );
        else throw CSyntaxError( SyntaxErrorType::UnknownRelation, _currentPosition );
    }
    else throw CSyntaxError( SyntaxErrorType::UnknownRelation, _currentPosition );
}

//
// Returns the next logical operator; doesn't handle NOT as it's parsed directly.
//
LogicalOper ConstraintsTokenizer::getLogicalOper()
{
    if      ( isNextSubstring( charArrToStr( TEXT_TokenLogicalOperAND ))) return ( LogicalOper::And );
    else if ( isNextSubstring( charArrToStr( TEXT_TokenLogicalOperOR  ))) return ( LogicalOper::Or );
    else return ( LogicalOper::Unknown );
}

//
// Parses parameter name
//
wstring ConstraintsTokenizer::getParameterName()
{
    wstring name;

    // look for opening marker
    if ( ! ( isNextSubstring( charArrToStr( TEXT_TokenParameterNameOpen ))))
    {
        throw CSyntaxError( SyntaxErrorType::NoParameterNameOpen, _currentPosition );
    }

    // retrive text
    name = getString( charArrToStr( TEXT_TokenParameterNameClose ));

    // look for closing marker
    if ( ! isNextSubstring( charArrToStr( TEXT_TokenParameterNameClose )))
    {
        throw CSyntaxError( SyntaxErrorType::NoParameterNameClose, _currentPosition );
    }

    return( name );
}

//
// Returns a number; reads from a string stream.
//
double ConstraintsTokenizer::getNumber()
{
    // declare new stream from text we'd like to parse
    //  then try to get numeric value preserving old and new
    //  position within a stream to properly update cursor
    wstring substring( _currentPosition, _constraintsText.end() );
    wistringstream ist( substring );

    unsigned int positionBefore = (unsigned int) ist.tellg();

    double number;
    ist>>number;

    if (ist.rdstate() & ios::failbit)
    {
        throw CSyntaxError( SyntaxErrorType::NotNumericValue, _currentPosition );
    }

    // success, update current cursor position
    unsigned int difference  =  (unsigned int) ist.tellg() - positionBefore;
    _currentPosition += difference;
    
    return ( number );
}

//
// Reads next characters considering them part of string
// Terminator is the enclosing char, typically a "
//
wstring ConstraintsTokenizer::getString( IN const wstring& terminator )
{
    wstring ret;

    assert( 1 == terminator.size() );
    wchar_t terminatingChar = terminator[ 0 ];

    wchar_t readChar;
    
    while( true )
    {
        // get next character, function throws error when there are no chars left
        readChar = peekNextChar();
        
        // string ends properly terminated
        if ( terminatingChar ==  readChar )
        {
            movePosition( -1 );
            break;
        }
        // handle special characters
        else if ( TEXT_SpecialCharMarker == readChar )
        {
            wchar_t nextChar = peekNextChar();

            bool found = false;
            for( auto specialChar : SpecialCharacters )
            {
                if( nextChar == specialChar ) found = true;
            }
            if( !found ) throw CSyntaxError( SyntaxErrorType::UnknownSpecialChar, _currentPosition );

            // found a special character; append to resulting string in literal form
            ret += nextChar;
        }
        // regular character: append to resulting string
        else
        {
            ret += readChar;
        }
    }

    return( ret );
}

//
// Skips all whitespace characters on and after current position.
//
void ConstraintsTokenizer::skipWhiteChars()
{
    // probe next character; the function throws error when there are no chars left),
    try
    {
        while ( true )
        {
            wchar_t nextChar = peekNextChar();
            
            if ( ! ( iswspace ( nextChar )   // all white space characters
                  || iswcntrl ( nextChar ))) // CRLF
            {
                movePosition( -1 );
                break;
            }
        }
    }
    // there's nothing wrong with encountering the end of string here;
    // other errors should be thrown to callers
    catch ( CSyntaxError e )
    {
        if ( SyntaxErrorType::UnexpectedEndOfString != e.Type )
        {
            throw e;
        }
    }
}

//
// Returns the next character updating the current position
// Throws when no more characters are left
//
wchar_t ConstraintsTokenizer::peekNextChar()
{
    if ( _currentPosition >= _constraintsText.end() )
    {
        throw CSyntaxError( SyntaxErrorType::UnexpectedEndOfString, _currentPosition );
    }
    return( *( _currentPosition++ ) );
}

//
// If texts match, returns True and also updates the current cursor position
// (unless explicitly requested not to)
//
bool ConstraintsTokenizer::isNextSubstring( IN const wstring& text, IN bool dontMoveCursor )
{
    skipWhiteChars();

    // Some STL implementations throw when text2 passed to 'equal' is shorter than text1.
    // Checking for the sizes first should help.
    bool textsMatch = false;

    if( distance( _currentPosition, _constraintsText.end() ) >= (int) text.size() )
    {
        textsMatch = equal ( text.begin(), text.end(), _currentPosition,
                             []( wchar_t c1, wchar_t c2 ) { return ( toupper( c1 ) == toupper( c2 ) ); }
                           );
    }

    if ( textsMatch && ! dontMoveCursor )
    {
        _currentPosition += text.length();
    }

    return ( textsMatch );
}

//
//
//
void ConstraintsTokenizer::movePosition( IN int count )
{
    wstring::iterator newPosition = _currentPosition + count;

    if ( newPosition < _constraintsText.begin() )
    {
        newPosition = _constraintsText.begin();
    }
    else if ( newPosition >= _constraintsText.end() ) 
    {
        newPosition = _constraintsText.end();
    }
    _currentPosition = newPosition;
}

//
// Expands "macros", there are two macros curently:
//   IsNegative() == ( IsNegative(p1) or  IsNegative(p2) or  ... )
//   IsPositive() == ( IsPositive(p1) and IsPositive(p2) and ... )
//
void ConstraintsTokenizer::doPostParseExpansions( IN OUT CTokenList& tokens )
{
    CTokenList::iterator i_token = tokens.begin();
    while( i_token != tokens.end() )
    {
        switch( (*i_token)->Type )
        {
        case TokenType::Function:
            {
            CFunction *function = (CFunction*) (*i_token)->Function;

            if(( function->Type == FunctionType::IsNegativeParam
              || function->Type == FunctionType::IsPositiveParam ) 
              && function->DataText.empty() )
            {
                // deallocate the current token
                // we don't have to deallocate Data because in this case it is always "nullptr"
                assert( function->Data == nullptr );

                // save positionInText and rawText and reuse it in all new tokens
                wstring::iterator oldPosInText = (*i_token)->PositionInText;
                FunctionType      oldType      = function->Type;
                wstring           oldRawText   = function->RawText;
                
                delete(*i_token);
                i_token = tokens.erase( i_token );
                
                // (
                CToken* newToken = new CToken( TokenType::ParenthesisOpen, oldPosInText );
                tokens.insert( i_token, newToken );

                for( CParameters::iterator i_param =  _model.Parameters.begin();
                                           i_param != _model.Parameters.end();
                                         ++i_param )
                {
                    if ( i_param->ResultParam ) continue;

                    if( i_param != _model.Parameters.begin() )
                    {
                        // logical operator OR or AND
                        newToken = new CToken( oldType == FunctionType::IsNegativeParam ? LogicalOper::Or : LogicalOper::And,
                                               oldPosInText );
                        tokens.insert( i_token, newToken );                    
                    }

                    // IsNegative(param) / IsPositive(param)
                    CFunction* newFunction = new CFunction( oldType, FunctionDataType::Parameter,
                                                            &*i_param, i_param->Name, oldRawText );
                    newToken = new CToken( newFunction, oldPosInText );
                    tokens.insert( i_token, newToken );
                }

                // )
                newToken = new CToken( TokenType::ParenthesisClose, oldPosInText );
                tokens.insert( i_token, newToken );
            }
            else // it's not IsNegative() or IsPositive()
            {
                ++i_token;
            }
            break;
            }
        default:
            {
            ++i_token;
            break;
            }
        }
    }
}

}
