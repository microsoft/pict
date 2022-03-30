#include <fstream>
#include <sstream>
#include "model.h"
using namespace std;

const wchar_t COMMENT_CHAR    = L'#';
const wchar_t PARAMNAME_SEP   = L':';
const wchar_t PARAM_ORDER     = L'@';
const wchar_t PARAM_REF_BEGIN = L'<';
const wchar_t PARAM_REF_END   = L'>';
const wchar_t WEIGHT_BEGIN    = L'(';
const wchar_t WEIGHT_END      = L')';
const wchar_t SET_BEGIN       = L'{';
const wchar_t SET_END         = L'}';
const wchar_t SET_ORDER       = L'@';
const wchar_t SET_SEP         = L','; // default separator of param names in submodel/cluster definition

const wchar_t RESULT_PARAM_PREFIX = L'$';

// note: keep it consistent with cpsyntax
// TODO: change the way we detect the constraints, this is error-prone
wstring CONSTRAINT_PATTERN1 = L"IF";
wstring CONSTRAINT_PATTERN2 = L"IF*[*]*";
wstring CONSTRAINT_PATTERN3 = L"[*]*";
wstring CONSTRAINT_PATTERN4 = L"(*[*]*";
wstring CONSTRAINT_PATTERN5 = L"IF*ISNEGATIVE";
wstring CONSTRAINT_PATTERN6 = L"IF*ISNEGATIVE*(*";
wstring CONSTRAINT_PATTERN7 = L"IF*ISPOSITIVE";
wstring CONSTRAINT_PATTERN8 = L"IF*ISPOSITIVE*(*";

//
//
//
bool lineIsComment( wstring& line )
{
    wstring trimmedLine = trim( line );
    if ( trimmedLine.empty() ) return( false );
    return( trimmedLine.at( 0 ) == COMMENT_CHAR );
}

//
// detects whether a line is a constraint
// TODO: have better detection here
//
bool lineIsConstraint( wstring& line )
{
    wstring trimmed = line;
    toUpper( trimmed );
    trimmed = trim( trimmed );

    // if the line contains just "IF", it is a constraint
    if( 0 == stringCompare( trimmed, CONSTRAINT_PATTERN1, false ) )
    {
        return( true );
    }

    // if the line matches any of the other patterns, it's a constraint
    return ( patternMatch( CONSTRAINT_PATTERN2, trimmed )
          || patternMatch( CONSTRAINT_PATTERN3, trimmed )
          || patternMatch( CONSTRAINT_PATTERN4, trimmed )
          || patternMatch( CONSTRAINT_PATTERN5, trimmed )
          || patternMatch( CONSTRAINT_PATTERN6, trimmed )
          || patternMatch( CONSTRAINT_PATTERN7, trimmed )
          || patternMatch( CONSTRAINT_PATTERN8, trimmed ) );
}

//
// detects whether a line is a submodel or a cluster definition
// must begin with { and must have } somewhere
//
bool lineIsParamSet( wstring& line )
{
    wstring trimmed = trim( line );

    if( trimmed.empty() )
    {
        return( false );
    }

    if( trimmed[ 0 ] != SET_BEGIN )
    {
        return( false );
    }
    
    size_t setend = trimmed.find( SET_END );
    if( wstring::npos == setend )
    {
        return( false );
    }
    
    return( true );
}

//
// reads one line from a file
//
bool readLineFromFile( wifstream& file, wstring& line )
{
    line = L"";
    if( file.eof() )
        return( false );

    wchar_t c;
    while( true )
    {
        file.get( c );
        if( file.eof()
         || c == L'\n'
         || c == L'\0' ) return( true );
        line += c;
    }
    return( true );
}

//
// read one parameter, these are in the following format:
// param [@ N] : val1, ~val2, val3a | val3b, val4
//
bool CModelData::readParameter( wstring& line )
{
    CModelParameter parameter;

    // param name can be separated by : or ,
    wstring::size_type paramSep = line.find( PARAMNAME_SEP );
    if( paramSep == wstring::npos )
    {
        paramSep = line.find( ValuesDelim );
        if( paramSep == wstring::npos )
        {
            PrintMessage( InputDataError, L"Parameter", line.c_str(), L"should have at least one value defined" );
            return( false );
        }
    }

    wstring name = trim( line.substr( 0, paramSep ));
    
    unsigned int order = static_cast<unsigned int>(UNDEFINED_ORDER);
    
    //check if this param has custom-order defined
    wstrings nameAndOrder;
    split( name, PARAM_ORDER, nameAndOrder );

    double d;
    if( nameAndOrder.size() == 2 && stringToNumber( nameAndOrder[ 1 ], d ))
    {
        name  = trim( nameAndOrder[ 0 ]);
        if( d > 0 )
        {
            order = static_cast< unsigned int >( d );
        }
    }

    parameter.Name  = name;
    parameter.Order = order;

    if ( ! parameter.Name.empty() && parameter.Name[ 0 ] == RESULT_PARAM_PREFIX )
    {
        parameter.IsResultParameter = true;
    }

    // now get the values
    wstring rawValues = line.substr( paramSep + 1, line.length() - paramSep - 1 );

    wstrings values;
    split( rawValues, ValuesDelim, values );

    for( wstrings::iterator i_val = values.begin(); i_val != values.end(); i_val++ )
    {
        *i_val = trim( *i_val );

        //
        // if it is in a form <text> it is a reference to another parameter
        // find an existing parameter and add all its values here instead
        //
        vector< CModelParameter >::iterator refParam;
        if ( ! i_val->empty() 
          && *(i_val->begin())  == PARAM_REF_BEGIN
          && *(i_val->rbegin()) == PARAM_REF_END
          &&( refParam = FindParameterByName( i_val->substr( 1, i_val->length() - 2 ))) !=
                         Parameters.end() )
        {
            __push_back( parameter.Values, refParam->Values.begin(), refParam->Values.end() );
        }
        else
        {
            //
            // value weight
            // Param: Val1 (3), Val21|Val22 (2), Val3
            //
            int weight = 1;
            
            size_t weightBegin = i_val->find_last_of( WEIGHT_BEGIN );
            size_t weightEnd   = i_val->find_last_of( WEIGHT_END );
            
            // '(' must exist, ')' must be the last character
            if ( weightBegin != wstring::npos && weightEnd == i_val->length() - 1 )
            {
                wstring weightStr = trim( i_val->substr( weightBegin + 1, weightEnd - weightBegin - 1 ));
                double weightDbl = 0;

                // anything after @ must be a positive integer
                if ( stringToNumber( weightStr, weightDbl ) && ( static_cast< unsigned int > (weightDbl) ) > 0 )
                {
                    weight = static_cast< unsigned int > (weightDbl);

                    // trim the weight off the value
                    i_val->erase( weightBegin, wstring::npos );
                    *i_val = trim( *i_val );
                }
            }

            //
            // names
            //
            wstrings names;
            split( *i_val, NamesDelim, names );

            bool positive = true;
            for ( wstrings::iterator i_name = names.begin(); i_name != names.end(); i_name++ )
            {
                *i_name = trim( *i_name );
                
                // only the first name determines the negativity of a value
                if ( i_name->length() > 0
                 &&  i_name == names.begin()
                 &&(*i_name)[ 0 ] == InvalidPrefix )
                {
                    positive = false;
                    *i_name = trim( i_name->substr( 1, i_name->length() - 1 ));
                }
            }

            if ( ! positive ) 
            {
                m_hasNegativeValues = true;
            }
            CModelValue value( names, weight, positive );
            parameter.Values.push_back( value );
        }
    }

    Parameters.push_back( parameter );
    return( true );
}

//
//
//
void CModelData::getUnmatchedParameterNames( wstrings& paramsOfSubmodel, wstrings& unmatchedParams )
{
    for( auto & cparam : paramsOfSubmodel )
    {
        bool found = false;
        for( auto & param : Parameters )
        {
            if ( 0 == stringCompare( cparam, param.Name, CaseSensitive ))
            {
                found = true;
                break;
            }
        }
        if ( ! found )
        {
            unmatchedParams.push_back( cparam );
        }
    }
}

//
//
//
bool CModelData::readParamSet( wstring& line )
{
    const wstring STD_MSG = L"Submodel definition is incorrect: " + line;

    wstringstream ist( line );

    // it's always in a form of { paramName1 @ N, paramName2 @ N, ... } @ N but "@ N" is optional

    wstring s;
    ist >> s;

    wstring::iterator next = line.begin();

    // {
    wstring::iterator begin = findFirstNonWhitespace( next, line.end() );
    if( begin == line.end() || *begin != SET_BEGIN )
    {
        PrintMessage( InputDataError, STD_MSG.data() );
        return( false );
    }
    ++begin;

    // find }
    wstring::iterator end;
    end = find( begin, line.end(), SET_END );
    if ( end == line.end() )
    {
        PrintMessage( InputDataError, STD_MSG.data() );
        return( false );
    }

    // params in the middle
    wstring setp;
    setp.assign( begin, end );
    setp = trim( setp );
    if ( setp.empty() )
    {
        PrintMessage( InputDataError, STD_MSG.data() );
        return( false );
    }

    //
    // Two attempts to resolve submodel names:
    // 1. Use a comma as a separator
    // 2. If 1 fails to produce matching names, use ModelData.ValuesDelim as a separator
    //

    // first figure out whether "," or a delimiter specified by /d option applies
    wstrings setParams;
    
    split( setp, SET_SEP, setParams );
    transform( setParams.begin(), setParams.end(), setParams.begin(), trim );

    wstrings unmatched;
    getUnmatchedParameterNames( setParams, unmatched );

    if( !unmatched.empty() )
    {
        setParams.clear();
        unmatched.clear();
        split( setp, ValuesDelim, setParams );
        transform( setParams.begin(), setParams.end(), setParams.begin(), trim );

        getUnmatchedParameterNames( setParams, unmatched );
        if( !unmatched.empty() )
        {
            PrintMessage( InputDataWarning, L"Submodel defintion", trim( line ).data(), L"contains unknown parameter. Skipping..." );
            return( true ); // just a warning so don't exit
        }
    }

    // remove duplicates
    sort( setParams.begin(), setParams.end(), stringCaseInsensitiveLess );
    wstrings::iterator newEnd = unique( setParams.begin(), setParams.end(), stringCaseInsensitiveEquals );
    if( setParams.end() != newEnd )
    {
        PrintMessage( InputDataWarning, L"Submodel defintion", trim( line ).data(), L"contains duplicate parameters. Removing duplicates..." );
        setParams.erase( newEnd, setParams.end() );
    }

    CModelSubmodel submodel;

    // match to names, set up the structure
    for( auto & cparam : setParams )
    {
        bool found = false;
        unsigned int index = 0;
        for( auto & param : Parameters )
        {
            if ( 0 == stringCompare( cparam, param.Name, CaseSensitive ))
            {
                found = true;
                break;
            }
            ++index;
        }
        // at this point we should always match the name
        assert( found );

        submodel.Parameters.push_back( index );
    }

    // @
    ++end;
    wstring::iterator at = findFirstNonWhitespace( end, line.end() );
    
    // anything other than @, quit
    if ( at != line.end() && *at != SET_ORDER )
    {
        PrintMessage( InputDataError, STD_MSG.data() );
        return( false );
    }

    if (  at == line.end() )
    {
        // if this is the end then order will be assigned later
        NOOP
    }
    else
    {
        ++at;

        // number
        wstring numberText;
        numberText.assign( at, line.end() );

        double number;
        bool ret = stringToNumber( numberText, number );
        
        int order = 0;
        if( ret )
        {
            order = static_cast<int> (number);
            if( order <= 0 )
            {
                order = 0;
                ret = false;
            }
        }
        if ( !ret )
        {
            PrintMessage( InputDataError, STD_MSG.data() );
            return( false );
        }

        submodel.Order = order;
    }

    Submodels.push_back( submodel );
    return ( true );
}


//
//
//
bool CModelData::readModel( const wstring& filePath )
{
    // Some implementations of wifstream only allow ANSI strings as file names so converting before using
    string ansiFilePath = wideCharToAnsi( filePath );
    wifstream file( ansiFilePath );
    if ( !file )
    {
        PrintMessage( InputDataError, L"Couldn't open file:", filePath.data() );
        return( false );
    }

    wstring line;

    // read definition of parameters
    bool firstLine = true;
    while( true )
    {
        // skip not important stuff
        if ( lineIsEmpty( line ) || lineIsComment( line ))
        {
            if ( ! readLineFromFile( file, line )) return( true );
            continue;
        }

        if ( firstLine )
        {
            m_encoding = getEncodingType( line );
            if ( m_encoding != EncodingType::ANSI
              && m_encoding != EncodingType::UTF8 )
            {
                PrintMessage( InputDataError, L"Only ANSI and UTF-8 are supported" );
                return( false );
            }
            firstLine = false;
        }

        // continue reading until a submodel/cluster or a constraint
        if ( lineIsParamSet( line ) || lineIsConstraint( line )) break;

        if ( ! readParameter( line ))          return( false );
        if ( ! readLineFromFile( file, line )) return( true );
    }

    // read submodels
    if ( lineIsParamSet( line ))
    {
        while( true )
        {
            // skip not important stuff
            if ( lineIsEmpty( line ) || lineIsComment( line ))
            {
                if ( ! readLineFromFile( file, line )) return( true );
                continue;
            }

            // continue reading until a constraint
            if ( lineIsConstraint( line )) break;

            if ( ! readParamSet( line ))           return( false );
            if ( ! readLineFromFile( file, line )) return( true );
        }
    }

    // anything that's left is constraints
    while( true )
    {
        // if only a line is not empty or not a comment,
        //   it's got to be a part of constraints definition
        if ( ! ( lineIsEmpty( line ) || lineIsComment( line )))
        {
            ConstraintPredicates += line;
        }  
        if ( ! readLineFromFile( file, line )) return( true );
    }

    return( true );
}

//
// reads model file
//
bool CModelData::ReadModel( const wstring& filePath )
{
    if( !readModel( filePath ))
    {
        return( false );
    }

    if( !ValidateParams() )
    {
        return( false );
    }

    return( true );
}

//
//
//
bool CModelData::ReadRowSeedFile( const wstring& filePath )
{
    if( trim( filePath ).empty() ) return( true );

    // Some implementations of wifstream only allow ANSI strings as file names so converting before using
    string ansiFilePath = wideCharToAnsi( filePath );
    wifstream file( ansiFilePath );
    if ( !file )
    {
        PrintMessage( InputDataError, L"Couldn't open file:", filePath.data() );
        return( false );
    }
    wstring line;

    // parameter names

    bool fileEmpty = false;
    if ( readLineFromFile( file, line ))
    {
        if ( trim( line ).empty() ) fileEmpty = true;
    }
    else
    {
        fileEmpty = true;
    }

    if ( fileEmpty )
    {
        PrintMessage( RowSeedsWarning, L"Seeding file is empty" ); 
        return( true );
    }

    EncodingType encoding = getEncodingType( line );
    if ( encoding != EncodingType::ANSI
      && encoding != EncodingType::UTF8 )
    {
        PrintMessage( RowSeedsError, L"Only ANSI and UTF-8 are supported" );
        return( false );
    }

    vector< vector<CModelParameter>::iterator > parameters;

    wstrings params;
    split( line, RESULT_DELIMITER, params );
    for( auto & param : params )
    {
        vector<CModelParameter>::iterator found = FindParameterByName( param );
        if ( found == Parameters.end())
        {
            PrintMessage( RowSeedsWarning, L"Parameter", 
                                           param.data(),
                                           L"not found in the model. Skipping..." );
        }
        parameters.push_back( found );
    }

    // if any parameter equals to ModelData.Parameters.end()
    // this parameter could not be found in the model

    while( readLineFromFile( file, line ))
    {
        if ( trim(line).empty() ) break;

        wstrings values;
        split( line, RESULT_DELIMITER, values );

        unsigned int n_param = 0;
        CModelRowSeed rowSeed;
        for ( wstrings::iterator i_value  = values.begin(); 
                                 i_value != values.end(); 
                               ++i_value, ++n_param )
        {
            // There could be fewer parameter names (in the first line)
            // than there is values in the following lines. This has
            // to be detected and a warning issued
            if ( n_param < (unsigned int) parameters.size() && parameters[ n_param ] != Parameters.end() )
            {
                CModelParameter &param = *(parameters[ n_param ]);
                
                // remove the negative marker and match up the raw name
                if ( i_value->length() > 0  && (*i_value)[ 0 ] == InvalidPrefix )
                {
                    *i_value = trim( i_value->substr( 1, i_value->length() - 1 ));
                }

                // if any value could not be found, the whole seed row is not invalid
                // we just remove that one offending value and the rest of the row can
                // stay intact; we cannot really warn about this as in a model with
                // submodels this is very normal
                int found = param.GetValueOrdinal( *i_value, CaseSensitive );
                if ( found == -1 )
                {
                    if ( ! i_value->empty() )
                    {
                        PrintMessage( RowSeedsWarning, L"Value", 
                                                    i_value->data(),
                                                    L"not found in the model. Skipping this value..." );
                    }
                }
                else
                {
                    // we don't care about result parameters as we should not seed we expected results
                    if ( ! param.IsResultParameter )
                    {
                        rowSeed.push_back( make_pair( param.Name, *i_value ));
                    }
                }
            }
        }
        if ( ! rowSeed.empty() )
        {
            RowSeeds.push_back( rowSeed );
        }
    }

    if( ! ValidateRowSeeds())
    {
        return( false );
    }

    return( true );
}
