#pragma once
#ifdef _MSC_VER
#pragma warning(disable:4511)
#pragma warning(disable:4512)
#pragma warning(disable:4239)
#endif

#include <vector>
#include <string>
#include <list>
#include <set>
#include <map>
#include <queue>
#include <deque>
#include <iostream>
#include <algorithm>
#include <functional>
#include <cassert>

//
// Logging facility
//

// #define _DOUT // output verbose info
// #define _FILE // output to generator.log, otherwise to console

#if defined(_DOUT)
#if defined(_FILE)
#include <fstream>
wofstream logfile;
#define DOUT(arg) logfile << arg
#else
#define DOUT(arg) wcerr << arg
#endif
#else
#define DOUT(arg)
#endif

//
// due to discrepancies between different implementations of STL, the following
// functions are defined explicitly and used throughout
//

//
// Insert a range into a collection
//
template<class _Col>
void __insert
    (
    _Col&                   collection,
    typename _Col::iterator first,
    typename _Col::iterator last
    )
{
    for ( ; first != last; ++first )
    {
        collection.insert( *first );
    }
}

//
// Push_back a range into a collection
//
template<class _Col>
void __push_back
    (
    _Col&                   collection,
    typename _Col::iterator first,
    typename _Col::iterator last
    )
{
    for ( ; first != last; ++first )
    {
        collection.push_back( *first );
    }
}

//
// Remove a node from a tree. Return the current node
//
template<class _Col>
typename _Col::iterator __map_erase
    (
    _Col&    collection,
    typename _Col::iterator where
    )
{
    if ( where == collection.end() ) return( collection.end() );
    typename _Col::iterator next = where;
    ++next;
    collection.erase( where );
    return( next );
}


#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

namespace pictcore
{

//
// forward references
//
class Task;
class Model;
class Parameter;
class Combination;
class Exclusion;
class WorkList;

//
// callback function cancelling the generation
//
typedef bool( *AbortCallbackFunc ) ( );

//
// internal constants and enums
//
const long MaxRowsToGenerate = 1000 * 1000; // could make this bigger

typedef unsigned char TrackType;

const TrackType OPEN = 0x00;
const TrackType COVERED = 0x01;
const TrackType EXCLUDED = 0xff;

enum ComboStatus
{
    Open,
    CoveredMatch,
    Excluded
};

enum class GenerationType
{
    MixedOrder,
    FixedOrder,
    Full,
    Flat,
    Random
};

enum class GenerationMode
{
    Regular,      // regular, no shortcuts, all pairs covered, can be slow
    Preview,      // only a few test cases, most pairs *not* covered, very fast
    Approximate   // most pairs covered, can return no results
};

enum class ErrorType
{
    GenerationCancelled,
    TooManyRows,
    GenerationFailure,
    OutOfMemory,
    Unknown
};

class GenerationError
{
public:

    GenerationError( std::string, int, ErrorType err = ErrorType::Unknown ) :
        _err( err ){}

    ErrorType GetErrorType() { return (ErrorType) _err; }

private:
    ErrorType    _err;
};

//
//
//
typedef std::vector<Combination *> ComboCollection;
typedef std::vector<Parameter *>   ParamCollection;
typedef std::list<size_t>          ParamResult;
typedef std::pair<Parameter *, std::wstring> Value;

//
// seeding structures
//
typedef std::pair<Parameter *,int> RowSeedTerm;
typedef std::set<RowSeedTerm>      RowSeed;
typedef std::list<RowSeed>         RowSeedCollection;

void printRowSeed( RowSeed &seed );

//
// results
//
typedef std::vector<size_t>    ResultRow;
typedef std::vector<ResultRow> ResultCollection;

//
// submodels
//
typedef std::list<Model *> SubmodelCollection;

//
// exclusions
//
typedef std::pair<Parameter *, int> ExclusionTerm;

int compareExclusionTerms( const ExclusionTerm& op1, const ExclusionTerm& op2 );
int compareExclusions( const Exclusion& op1, const Exclusion& op2 );
extern bool contained( Exclusion &a, Exclusion &b );

// exclusions have to be sorted by size when they enter ProcessExclusions()
// otherwise correct combinations will not be created and generation will fail
class ExclusionSizeLess
{
public:
    bool operator() ( const Exclusion &excl1, const Exclusion &excl2 ) const;
};
typedef std::set<Exclusion, ExclusionSizeLess> ExclusionCollection;

class ExclIterCollectionPred
{
public:
    bool operator() ( const ExclusionCollection::iterator excl1, const ExclusionCollection::iterator excl2 ) const;
};
typedef std::set<ExclusionCollection::iterator, ExclIterCollectionPred> ExclIterCollection;

class ExclusionTermCompare
{
public:
    bool operator()( const ExclusionTerm& op1, const ExclusionTerm& op2 ) const;
};

//
//
//
class Exclusion
{
public:
    typedef std::set<ExclusionTerm, ExclusionTermCompare> _Exclusion;
    typedef std::vector<ExclusionTerm>                    _ExclusionVec;
    typedef _Exclusion::iterator                          iterator;
    typedef _Exclusion::const_iterator                    const_iterator;
    typedef _Exclusion::reference                         reference;
    typedef _Exclusion::const_reference                   const_reference;
    typedef _Exclusion::value_type                        value_type;

    Exclusion() : m_deleted( false ) {}

    iterator       begin()       { return( col.begin() ); }
    iterator       end()         { return( col.end() ); }
    const_iterator begin() const { return( col.begin() ); }
    const_iterator end()   const { return( col.end() ); }
    size_t size()  const         { return( col.size() ); }
    bool   empty() const         { return( col.empty() ); }

    // exclusion represented as a list, for next_permutation to work
    _ExclusionVec::iterator lbegin() { return( vec.begin() ); }
    _ExclusionVec::iterator lend()   { return( vec.end() ); }
    _ExclusionVec &GetList()         { return( vec ); }

    std::pair<iterator, bool> insert( const ExclusionTerm& Term )
    {
        std::pair<iterator, bool> ret = col.insert( Term );
        if( ret.second ) vec.push_back( Term );
        assert( col.size() == vec.size() );
        return ret;
    }

    // for inserter() to work we need two-param insert
    iterator insert( iterator, const ExclusionTerm& Term )
    {
        return insert( Term ).first;
    }

    bool operator <( const Exclusion& Excl ) const {
        return( size() != Excl.size() ? size() < Excl.size() : -1 == compareExclusions( *this, Excl ) );
    }

    void markDeleted() { m_deleted = true; }
    bool isDeleted() const { return( m_deleted ); }

    size_t ResultParamCount() const;

    void Print() const;

private:
    _Exclusion    col;
    _ExclusionVec vec;
    bool m_deleted;
};

//
// combination
//
const unsigned int UNDEFINED_ID = 0;

class Combination
{
public:
    Combination( Model* model );
    ~Combination();

    static void ResetId() { m_lastUsedId = UNDEFINED_ID; }
    unsigned int GetId()  { return m_id; }

    void WireModel( Model* model ) { m_model = model; }

    void PushParameter( Parameter* param ) { m_params.push_back( param ); }
    void PopParameter()                    { m_params.pop_back(); }
    Parameter& operator[]( int N ) const   { return *( m_params[ N ] ); }

    void InitBinding() { m_boundCount = 0; }
    int  Bind( int val, WorkList& worklist );
    int  AddBinding();
    int  GetBoundCount() const { return m_boundCount; }
    bool IsFullyBound()  const { return m_boundCount == static_cast<int>(m_params.size()); }

    void        SetOpen   ( int n );
    bool        IsOpen    ( int n ) const { return OPEN     == m_bitvec[ n ]; }
    bool        IsExcluded( int n ) const { return EXCLUDED == m_bitvec[ n ]; }
    ComboStatus Feasible  ( int n );
    int         Feasible();

    void ApplyExclusion( Exclusion& excl );
    bool ViolatesExclusion();

    ParamCollection& GetParameters() { return m_params; }
    int  GetParameterCount() const { return static_cast<int>( m_params.size() ); }
    int  GetOpenCount()      const { return m_openCount; }
    int  GetRange()          const { return m_range; }
    void SetMapSize( int n, TrackType val = 0 );
    int  Weight( int vak );
    void Print();

    void Assign( Combination &Combo )
    {
        m_params     = Combo.GetParameters();
        m_range      = Combo.GetRange();
        m_openCount  = Combo.GetOpenCount();
        m_boundCount = Combo.GetBoundCount();
    }

private:
    static unsigned int m_lastUsedId; // static source of identifiers
    unsigned int        m_id;         // unique identifier of this instance

    ParamCollection m_params;
    unsigned char*  m_bitvec;
    int             m_range;
    int             m_openCount;
    int             m_boundCount;

    Model* m_model;

    void applyExclusion( Exclusion& excl, int index, ParamCollection::iterator pos );
};

// Combinations pointers in ComboCollection should be sorted by id and not by memory location
// We want to avoid any indeterminism stemming from sorting random pointers
class CombinationPtrSortPred {
public:
    bool operator() (Combination *c1, Combination *c2)
    {
        return(c1->GetId() < c2->GetId());
    }
};

//
// parameter
//
class Parameter
{
public:
    Parameter( int order, int sequence, int valueCount, std::wstring name, bool expectedResultParam ) :
        m_name( name ), m_order( order ), m_sequence( sequence ), m_valueCount( valueCount ),
        m_expResultParam( expectedResultParam ), m_bound( false ),
        m_pending( false ), m_valueWeights( 0 ), m_avgExclusionSize( 0 )
    {
         // result params must have order = 1
        if ( m_expResultParam ) m_order = 1;
    }
    virtual ~Parameter() {}

    static const unsigned int UndefinedValue = 0xFFFFFFFF;

    void SetOrder( int Order ) { m_order = Order; }
    void SetWeights( std::vector<int> Weights );

    const std::wstring& GetName() const { return m_name; }
    ExclIterCollection& GetExclusions() { return m_exclusions; }

    int GetOrder() const { return m_order; }

    int GetWeight( int n ) const
    {
        if( 0 <= n && n < static_cast<int>( m_valueWeights.size() ) )
            return m_valueWeights[ n ];
        else
            return 1;
    }
    int GetSequence()        const { return m_sequence; }
    int GetValueCount()      const { return static_cast<int>( m_valueCount ); }
    int GetTempResultCount() const { return static_cast<int>( m_result.size() ); }
    int GetExclusionCount()  const { return static_cast<int>( m_exclusions.size() ); }
    void  ClearExclusions() { m_avgExclusionSize = 0; m_exclusions.clear(); }
    float GetAverageExclusionSize() const { return m_avgExclusionSize; }

    bool  IsExpectedResultParam() { return( m_expResultParam ); }

    void WireTask( Task* task ) { m_task = task; }

    void InitBinding()
    {
        m_bound   = false;
        m_pending = false;
    }
    bool Bind( int value, WorkList& worklist );
    bool GetBoundCount() const { return m_bound; }

    void MarkPending()     { m_pending = true; }
    bool IsPending() const { return m_pending; }

    int PickValue();

    void LinkCombination( Combination* combo ) { m_combinations.push_back( combo ); }
    
    // remember to sort by Id, not with default sorting pred (by memory address)
    void SortCombinations() { sort( m_combinations.begin(), m_combinations.end(), CombinationPtrSortPred() ); }
    ComboCollection::const_iterator GetCombinationBegin() { return m_combinations.begin(); }
    ComboCollection::const_iterator GetCombinationEnd()   { return m_combinations.end(); }

    void LinkExclusion( ExclusionCollection::iterator iter )
    {
        m_avgExclusionSize = ( m_avgExclusionSize * (float) m_exclusions.size() + (float) iter->size() ) / (float) ( m_exclusions.size() + 1 );
        std::pair<ExclIterCollection::iterator, bool> ret = m_exclusions.insert( iter );
        assert( ret.second );
    }

    void UnlinkExclusion( ExclusionCollection::iterator Iter )
    {
        size_t erased = m_exclusions.erase( Iter );
        assert( 1 == erased );
    }

    void GetFirst()   { m_resultIter = m_result.begin(); }
    size_t GetNext()  { return *m_resultIter++; }
    size_t GetLast()  { return  m_currentValue; }
    ParamResult& GetTempResults() { return m_result; }

    virtual Model*           GetModel()      { return nullptr; }
    virtual ParamCollection* GetComponents() { return nullptr; }

    void CleanUp();

protected:
    std::wstring m_name;

private:
    int    m_order;          // how many ways must this parameter combine?
    int    m_sequence;       // input sequence number, so we can reorder but output in original order
    size_t m_currentValue;   // current iteration's value cache
    int    m_valueCount;     // how many values parameter has
    bool   m_expResultParam; // is this a special, result inducing param

    bool m_bound;         // variable to use in iteration - is this bound yet?
    bool m_pending;       // have we put it on work list yet this iteration?

    ComboCollection       m_combinations;  // combinations this parameter participates in
    ExclIterCollection    m_exclusions;    // exclusions referring to this parameter

    ParamResult::iterator m_resultIter;
    ParamResult           m_result;

    std::vector<int>      m_valueWeights;

    Task*  m_task;

    float  m_avgExclusionSize;
};

//
// pseudoParameter is a parameter constructed out of results generated by a submodel
//
class PseudoParameter : public Parameter
{
public:
    PseudoParameter(int order, unsigned int sequence, Model *submodel);

    virtual Model*           GetModel() {return m_model;}
    virtual ParamCollection* GetComponents();

private:
    Model* m_model;
};

//
//
//
class Model
{
public:
    Model( const std::wstring& id, GenerationType type, int order, long seed = 0 ) :
        m_id( id ), m_order( order ), m_maxRows( 0 ), m_generationType( type ), m_lastParamId( UNDEFINED_ID + 1000 * 1000 ) {
        SetRandomSeed( seed );
    }
    ~Model();

    void GenerateVirtualRoot();  // generates a root of a hierarchical cluster
    void Generate();             // entry point of the generation
    GenerationType GetGenerationType() { return( m_generationType ); }

    bool AddExclusion( Exclusion& e )
    {
        std::pair<ExclusionCollection::iterator, bool> ret = m_exclusions.insert( e );
        return( ret.second );
    }
    void AddSubmodel( Model* model ) { m_submodels.push_back( model ); }
    
    void AddParameter( Parameter* param )
    {
        // parameter names should be unique but we can't assert it here
        // as there are cases where we add same name params to the collection
        // internally and delete the duplicate almost immediately
        param->WireTask( GetTask() );
        m_parameters.push_back( param );
    }

    void AddRowSeed( RowSeed& seed )
    {
        m_rowSeeds.push_back( seed );
        for( auto & submodel : m_submodels ) submodel->AddRowSeed( seed );
    }

    void AddRowSeeds( RowSeedCollection::iterator begin, RowSeedCollection::iterator end )
    {
        while( begin != end ) AddRowSeed( *begin++ );
    }

    void SetOrder     ( int order )    { m_order = order; }
    void SetMaxRows   ( long maxRows ) { m_maxRows = maxRows; }
    void SetRandomSeed( long seed )
    {
        m_randomSeed = seed;
        srand( m_randomSeed );
        for( auto & submodel : m_submodels ) submodel->SetRandomSeed( m_randomSeed );
    }

    void WireTask( Task* task )
    {
        m_task = task;
        for( auto & param : m_parameters )   param->WireTask( task );
        for( auto & submodel : m_submodels ) submodel->WireTask( task );
    }

    Task* GetTask() { return( m_task ); }

    std::wstring&         GetId()         { return m_id; }
    SubmodelCollection&   GetSubmodels()  { return m_submodels; }
    ExclusionCollection&  GetExclusions() { return m_exclusions; }
    RowSeedCollection&    GetRowSeeds()   { return( m_rowSeeds ); }
    ResultCollection&     GetResults()    { return m_results; }
    ParamCollection&      GetParameters() { return m_parameters; }

    void GetAllParameters( ParamCollection& params )
    {
        __push_back( params, m_parameters.begin(), m_parameters.end() );
        for( auto & submodel : m_submodels ) submodel->GetAllParameters( params );
    }

    size_t GetAllParametersCount()
    {
        size_t count = m_parameters.size();
        return( count );
    }

    size_t GetResultParameterCount()
    {
        size_t count = 0;
        for( auto & param : m_parameters )
            if( param->IsExpectedResultParam() )
                ++count;

        return count;
    }

    int  GetOrder()      { return m_order; }
    long GetRandomSeed() { return( m_randomSeed ); }

    long GetTotalCombinationsCount()     { return static_cast<long>( m_totalCombinations ); }
    long GetRemainingCombinationsCount() { return static_cast<long>( m_remainingCombinations ); }
    int  GetSubmodelCount()              { return static_cast<int> ( m_submodels.size() ); }
    int  GetResultCount()                { return static_cast<int> ( m_results.size() ); }

    int GlobalZerosCount;

private:

    ParamCollection        m_parameters;
    ExclusionCollection    m_exclusions;
    SubmodelCollection     m_submodels;
    RowSeedCollection      m_rowSeeds;
    std::deque<Parameter*> m_worklist;
    ResultCollection       m_results;

    std::wstring m_id;

    int  m_order;
    long m_randomSeed;
    long m_maxRows;

    GenerationType m_generationType;

    unsigned int m_lastParamId;
    long         m_totalCombinations;
    long         m_remainingCombinations;

    Task* m_task;

    void generateMixedOrder();           // generates mixed-order (the most generic)
    void generateFixedOrder();           // generates fixed, n-order
    void generateFull();                 // generates exhaustive
    void generateFlat();                 // generates order of 1 with extra assumptions
    void generateRandom();               // generates randomized suites

    void resolvePseudoParams();          // helper that handles submodels
    void fixRowSeeds();                  // helper that cleans up seeding rows
    void gcd( ComboCollection& ComboCol ); // generation entry point

    void choose( ParamCollection::iterator first,
                 ParamCollection::iterator last,
                 int order, int realOrder,
                 Combination& baseCombo, ComboCollection& vecCombo );

    void processExclusions( ComboCollection& comboCol );
    bool excludeConflictingParamValues();
    bool mapExclusionsToPseudoParameters();
    void mapRowSeedsToPseudoParameters();
    void deriveSubmodelExclusions();
    bool rowViolatesExclusion( ResultRow& row );
    bool rowViolatesExclusion( Exclusion& row );

    void markUndefinedValuesInResultParams();

    Exclusion generateRandomRow();
};

//
// task governs the entire generation
//
class Task
{
public:
    Task();
    ~Task();

    // this has to be called right before generation starts
    void PrepareForGeneration();

    void AllocWorkbuf( int Size );

    int* GetWorkbuf() { return( m_workbuf ); }

    void DeallocWorkbuf();

    void SetRootModel( Model* model )
    {
        m_rootModel = model;
        model->WireTask( this );
    }

    size_t GetTotalParameterCount() { return( m_rootModel->GetAllParametersCount() ); }

    void SetAbortCallback( AbortCallbackFunc func ) { m_abortCallback = func; }
    
    bool AddExclusion( Exclusion& excl )
    {
        std::pair<ExclusionCollection::iterator, bool> ret = m_exclusions.insert( excl );
        return( ret.second );
    }

    void AddRowSeed( RowSeed& seed ) { m_rowSeeds.push_back( seed ); }

    Model* GetRootModel()                { return( m_rootModel ); }
    ExclusionCollection& GetExclusions() { return( m_exclusions ); }
    RowSeedCollection&   GetRowSeeds()   { return( m_rowSeeds ); }

    bool AbortGeneration() { return( ( nullptr == m_abortCallback ) ? false : m_abortCallback() ); }

    void SetGenerationMode( GenerationMode mode ) { m_generationMode = mode; }
    GenerationMode GetGenerationMode() { return( m_generationMode ); }

    // approximate mode
    void SetMaxRandomTries( size_t max ) { m_maxRandomTries = max; }
    size_t GetMaxRandomTries() { return( m_maxRandomTries ); }

    ResultCollection& GetResults() { return m_rootModel->GetResults(); }

    // these two functions are used by C-style API which only returns one row at a time
    void ResetResultFetching();
    ResultCollection::iterator GetNextResultRow();

private:
    Model*              m_rootModel;
    ExclusionCollection m_exclusions;
    RowSeedCollection   m_rowSeeds;

    AbortCallbackFunc   m_abortCallback;
    GenerationMode      m_generationMode;

    // in Approximate and Randomized mode, how many times we will try to
    // generate a random row before giving up
    size_t              m_maxRandomTries; 

    Model* findMatchingNode( Exclusion& exclusion, Model* root );
    bool findParamInSubtree( Parameter* param, Model* root );
    void deriveExclusions();

    // a global workspace shared by multiple objects
    int* m_workbuf = nullptr;

    // result row pointer allows C-style API to implement GetNextResultRow function
    // i.e. get one result row at a time
    ResultCollection::iterator m_currentResultRow;
};

//
//
//
class WorkList 
{
public:
    WorkList() {}
    ~WorkList() {}

    void AddItem( Parameter* pParam );
    Parameter *GetItem();

    bool IsEmpty() const { return m_rep.empty(); }
    void Print();

private:
    std::deque<Parameter*> m_rep;
};

}

#ifdef __cplusplus
}
#endif // __cplusplus
