#pragma once

#include <vector>
#include <string>
#include <limits>
#include <fstream>
#include "generator.h"
#include "strings.h"
using namespace pictcore;

const int UNDEFINED_ORDER = std::numeric_limits<int>::min();
const int MAXIMUM_ORDER = std::numeric_limits<int>::max();

//
//
//
class CModelValue
{
public:
    CModelValue
        (
        wstrings&    names,
        unsigned int weight,
        bool         positive
        ) : _names           ( names ),
            _positive        ( positive ),
            _weight          ( weight ),
            _currentNameIndex( 0 ) {}

    wstrings& GetAllNames()       { return( _names ); }
    std::wstring GetPrimaryName() { return( _names[ 0 ] ); }

    // type of the value and any other attribute is currently decided based on the primary name
    wstrings GetNamesForComparisons();

    // round-robin through names
    std::wstring GetNextName();

    bool IsPositive() { return( _positive ); }
    unsigned int GetWeight() { return( _weight ); }

private:
    wstrings            _names;
    bool                _positive;
    unsigned int        _weight;
    wstrings::size_type _currentNameIndex;
};

//
//
//
class CModelParameter
{
public:
    std::wstring               Name;
    std::vector< CModelValue > Values;
    unsigned int               Order;             // default order assigned when parameter is defined
    bool                       IsResultParameter; // special parameter for results
    Parameter*                 GcdPointer;

    CModelParameter() :
        Name(L""),
        Order(static_cast<unsigned int>(UNDEFINED_ORDER)),
        IsResultParameter(false),
        GcdPointer(nullptr){}

    int GetValueOrdinal( IN std::wstring& name, IN bool caseSensitive );
    bool ValueNamesUnique( IN bool CaseSensitive );
};

//
//
//
typedef std::vector< unsigned int > CParameters;

class CModelSubmodel
{
public:
    int         Order;      // order at which a submodel should be combined
    CParameters Parameters; // indexes to Parameters in CModelData

    CModelSubmodel() : Order(UNDEFINED_ORDER) {}
};

//
//
//
typedef std::list< std::pair< std::wstring, std::wstring > > CModelRowSeed; // param_name, value_name
typedef std::vector< CModelRowSeed >                         CModelRowSeeds;

//
//
//
class CModelData
{
public:
    int                       Order;
    wchar_t                   ValuesDelim;
    wchar_t                   NamesDelim;
    wchar_t                   InvalidPrefix;
    unsigned short            RandSeed;
    bool                      CaseSensitive;
    bool                      Verbose;         // prints out some additional info while generating
    bool                      Statistics;      // show the statistics only
    pictcore::GenerationMode  GenerationMode;
    size_t                    MaxApproxTries;  // for Approximate mode

    std::wstring                   RowSeedsFile;

    std::vector< CModelParameter > Parameters;
    std::vector< CModelSubmodel >  Submodels;
    std::wstring                   ConstraintPredicates;
    std::vector< CModelRowSeed >   RowSeeds;

    std::set< wchar_t >            ProvidedArguments; // arguments defined by a user   
                                                      // helpful to catch redefinitions
    // ctor
    CModelData() : Order(2),
        ValuesDelim(L','),
        NamesDelim(L'|'),
        InvalidPrefix(L'~'),
        RandSeed(0),
        CaseSensitive(false),
        Verbose(false),
        Statistics(false),
        GenerationMode(GenerationMode::Regular),
        MaxApproxTries(1000),
        RowSeedsFile(L""),
        ConstraintPredicates(L""),
        m_hasNegativeValues(false),
        m_encoding(EncodingType::ANSI),
        m_totalCombinations(0),
        m_remainingCombinations(0) {}

    bool ReadModel      ( const std::wstring& filePath );
    bool ReadRowSeedFile( const std::wstring& filePath );

    size_t TotalParameterCount() { return(Parameters.size()); }
    size_t ResultParameterCount();

    std::vector< CModelParameter >::iterator FindParameterByName( const std::wstring& Name );
    std::vector< CModelParameter >::iterator FindParameterByGcdPointer( Parameter* Pointer );

    std::wstring GetConstraintText( unsigned int index );
    bool HasNegativeValues()   { return( m_hasNegativeValues ); }
    EncodingType GetEncoding() { return( m_encoding ); }
    void AddToTotalCombinationsCount    ( size_t n ) { m_totalCombinations += n; }
    void AddToRemainingCombinationsCount( size_t n ) { m_remainingCombinations += n; }

    void RemoveNegativeValues();

    bool ValidateParams();
    bool ValidateRowSeeds();

    void PrintModelContents( std::wstring title );
    void PrintStatistics();
    
private:
    bool            m_hasNegativeValues;
    EncodingType    m_encoding;              // io encoding determined based on input file
    size_t          m_totalCombinations;     // number of combinations PICT dealt with in this run
    size_t          m_remainingCombinations; // number of uncovered combinations (Preview and Approximate)

    void readFile                  ( const std::wstring& filePath );
    bool readModel                 ( const std::wstring& filePath );
    bool readParameter             ( std::wstring& line );
    bool readParamSet              ( std::wstring& line );
    void getUnmatchedParameterNames( wstrings& paramsOfSubmodel, wstrings& unmatchedParams );
};
