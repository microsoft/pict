#include <sstream>
#include "model.h"
using namespace std;

// ****************************************************************************
//
// CModelValue
//
// ****************************************************************************

//
// type of the value and any other attribute is currently decided based on the primary name
//
wstrings CModelValue::GetNamesForComparisons()
{
    wstrings names;
    names.push_back( GetPrimaryName() );
    return( names );
}

//
// round-robin through names
//
std::wstring CModelValue::GetNextName()
{
    if( _currentNameIndex == _names.size() )
    {
        _currentNameIndex = 0;
    }
    return( _names[ _currentNameIndex++ ] );
}

// ****************************************************************************
//
// CModelParameter
//
// ****************************************************************************

//
// returns an ordinal number of the first value with any matching name
// -1 if not found
//
int CModelParameter::GetValueOrdinal( wstring& findName, bool caseSensitive )
{
    int idx = 0;
    for( auto & value : Values )
    {
        for( auto name : value.GetAllNames() )
        {
            if( 0 == stringCompare( name, findName, caseSensitive ) ) return( idx );
        }
        ++idx;
    }
    return( -1 );
}

//
//
//
bool CModelParameter::ValueNamesUnique( IN bool caseSensitive )
{
    wstrings allNames;
    for( auto & value : Values )
    {
        allNames.insert( allNames.end(), value.GetAllNames().begin(), value.GetAllNames().end() );
    }

    wstrings::iterator newEnd;
    if( caseSensitive )
    {
        sort( allNames.begin(), allNames.end(), stringCaseSensitiveLess );
        newEnd = unique( allNames.begin(), allNames.end(), stringCaseSensitiveEquals );
    }
    else
    {
        sort( allNames.begin(), allNames.end(), stringCaseInsensitiveLess );
        newEnd = unique( allNames.begin(), allNames.end(), stringCaseInsensitiveEquals );
    }
    return( newEnd == allNames.end() );
}

// ****************************************************************************
//
// CModelData
//
// ****************************************************************************

//
//
//
size_t CModelData::ResultParameterCount()
{
    size_t count = 0;
    for( auto param : Parameters )
    {
        if( param.IsResultParameter ) ++count;
    }
    return( count );
}

//
//
//
vector< CModelParameter >::iterator CModelData::FindParameterByName( const wstring& name )
{
    for( vector< CModelParameter >::iterator i_param =  Parameters.begin();
                                             i_param != Parameters.end();
                                             i_param++ )
    {
        if( 0 == stringCompare( i_param->Name, name, CaseSensitive ) ) return( i_param );
    }
    return( Parameters.end() );
}

//
//
//
vector< CModelParameter >::iterator CModelData::FindParameterByGcdPointer( Parameter* ptr )
{
    for( vector< CModelParameter >::iterator i_param = Parameters.begin();
                                             i_param != Parameters.end();
                                             i_param++ )
    {
        if( i_param->GcdPointer == ptr ) return( i_param );
    }

    return( Parameters.end() );
}

//
// TODO: this is a quick-and-dirty way, should be changed
//
wstring CModelData::GetConstraintText( unsigned int index )
{
    wstrings constraints;
    split( ConstraintPredicates, TEXT_TokenConstraintEnd[ 0 ], constraints );

    return( constraints[ index ] + TEXT_TokenConstraintEnd );
}

//
//
//
void CModelData::RemoveNegativeValues()
{
    for( auto & param : Parameters )
    {
        vector< CModelValue > newValues;
        for( auto & value : param.Values )
        {
            bool positive = value.IsPositive();
            if( positive )
            {
                CModelValue newValue( static_cast<wstrings&> ( value.GetAllNames() ),
                                      value.GetWeight(), positive );
                newValues.push_back( newValue );
            }
        }
        param.Values = newValues;
    }
    m_hasNegativeValues = false;
}

//
//
//
bool CModelData::ValidateParams()
{
    //
    // parameter names should be unique
    //
    for( vector< CModelParameter >::iterator i_param1 = this->Parameters.begin();
                                             i_param1 != this->Parameters.end();
                                           ++i_param1 )
    {
        for( vector< CModelParameter >::iterator i_param2 = i_param1 + 1;
                                                 i_param2 != this->Parameters.end();
                                               ++i_param2 )
        {
            if( 0 == stringCompare( i_param1->Name, i_param2->Name, this->CaseSensitive ) )
            {
                PrintMessage( InputDataError, L"A parameter names must be unique" );
                return( false );
            }
        }
    }

    //
    // all values of one param can't be negative
    //
    for( auto & param : Parameters )
    {
        bool anyPositive = false;
        for( auto & value : param.Values )
        {
            if( value.IsPositive() )
            {
                anyPositive = true;
            }
        }

        if( !anyPositive )
        {
            PrintMessage( InputDataError, L"A parameter cannot have only negative values" );
            return( false );
        }
    }

    return( true );
}

//
// Warn if for any of the row seeds, a param or value name is blank or
// names of values are not unique which may lead to ambiguities later
//
bool CModelData::ValidateRowSeeds()
{
    for( auto & param : Parameters )
    {
        if( ( param.Name ).empty() || ( param.Name ).find( RESULT_DELIMITER ) != wstring::npos )
        {
            PrintMessage( RowSeedsWarning, L"Name of parameter",
                          param.Name.data(),
                          L"is blank or contains a tab character. Seeding may not work properly." );
        }

        for( auto & value : param.Values )
        {
            for( auto & name : value.GetAllNames() )
            {
                if( name.empty() || name.find( RESULT_DELIMITER ) != wstring::npos )
                {
                    PrintMessage( RowSeedsWarning, L"Value",
                                  value.GetPrimaryName().data(),
                                  L"or one of its names is blank or contains a tab character. Seeding may not work properly." );
                }
            }
        }

        if( !param.ValueNamesUnique( this->CaseSensitive ) )
        {
            PrintMessage( RowSeedsWarning, L"Values of the parameter",
                          param.Name.data(),
                          L"are not unique. Seeding may not work properly." );
        }
    }

    return( true );
}

//
//
//
void CModelData::PrintStatistics()
{
    PrintStatisticsCaption( wstring( L"Combinations" ) );
    wcout << m_totalCombinations << endl;

    if( GenerationMode == GenerationMode::Approximate )
    {
        size_t covered = m_totalCombinations - m_remainingCombinations;
        PrintStatisticsCaption( wstring( L"Covered" ) );
        wcout << covered << L" (" << covered * 100 / m_totalCombinations << L"%)" << endl;
    }
}

//
//
//
void CModelData::PrintModelContents( wstring title )
{
    PrintLogHeader( title );

    PrintLogHeader( L"Parameter summary" );
    wcerr << L"Model has " << (unsigned int) TotalParameterCount() << L" parameters," << endl;
    wcerr << L"including " << (unsigned int) ResultParameterCount() << L" result parameters:" << endl;

    for( auto & param : Parameters )
    {
        wcerr << L" " << param.Name << L":\t" << (unsigned int) param.Values.size() << L" values, order: ";
        if( nullptr == param.GcdPointer )
            wcerr << L"?" << endl;
        else
            wcerr << param.Order << L" : " << param.GcdPointer->GetOrder() << endl;
    }

    PrintLogHeader( L"Submodel summary" );
    for( auto & item : Submodels )
    {
        for( auto & param : item.Parameters )
        {
            wcerr << L" " << static_cast< unsigned int >( param );
        }
        wcerr << L" @ " << item.Order << endl;
    }

    PrintLogHeader( L"Row seeds summary" );
    for( auto & seed : RowSeeds )
    {
        for( auto & item : seed )
        {
            wcerr << L"[" << item.first << L": " << item.second << L"] ";
        }
        wcerr << endl;
    }
}
