#include "generator.h"
#include "deriver.h"

namespace pictcore
{

const size_t DefaultMaxRandomTries = 1000;

//
//
//
Task::Task() :
      m_rootModel     (nullptr),
      m_abortCallback (nullptr),
      m_generationMode(GenerationMode::Regular),
      m_maxRandomTries(DefaultMaxRandomTries),
      m_workbuf       (nullptr)
{
    Combination::ResetId();

#if ( defined(_DOUT) || defined(_FILE) )
    wcerr << L"WARNING: _DOUT or _FILE are defined\n";
#endif

#if ( defined(_DOUT) && defined(_FILE) )
    logfile.open( "\\generator.log", ofstream::out );
#endif
}

//
//
//
Task::~Task()
{
    DeallocWorkbuf();

#if ( defined(_DOUT) && defined(_FILE) )
    logfile.close();
#endif
}

//
//
//
void Task::AllocWorkbuf( int size )
{
    DeallocWorkbuf();
    m_workbuf = new int[ size ];
}

//
//
//
void Task::DeallocWorkbuf()
{
    if( nullptr != m_workbuf )
    {
        delete[] m_workbuf;
        m_workbuf = nullptr;
    }
}

//
//
//
void Task::PrepareForGeneration()
{
    // new models could have been added to the root
    // repropagate the task pointer to all models
    m_rootModel->WireTask(this);

    // run the deriver on exclusions
    deriveExclusions();

    // propagate exclusions wiring to appropriate submodel
    // add each exclusion to that model in the hierarchy which is the most suitable:
    //  1. a subtree of that model must have all the parameters of the exclusion
    //  2. no lower subtree satisfies the condition 1)
    //
    for ( auto & excl : m_exclusions )
    {
        Model* found = findMatchingNode(const_cast<Exclusion&>( excl ), m_rootModel);
        assert( nullptr != found );
        found->AddExclusion( const_cast<Exclusion&>( excl ));
    }

    // propagate all row seeds
    m_rootModel->AddRowSeeds( m_rowSeeds.begin(), m_rowSeeds.end() );
}

//
//
//
void Task::ResetResultFetching()
{
    m_currentResultRow = m_rootModel->GetResults().begin();
}

//
//
//
ResultCollection::iterator Task::GetNextResultRow()
{
    ResultCollection::iterator ret = m_currentResultRow;
    if( m_currentResultRow != GetResults().end() )
    {
        ++m_currentResultRow;
    }
    return( ret );
}

//
// for a given exclusion, this routine finds the best fitting model in a hierarchy:
// the one that contains all params of the exclusion in its subtree and 
// none of its children have this property
//
Model* Task::findMatchingNode( Exclusion& exclusion, Model* root )
{
    // can any of the subnodes accommodate the exclusion?
    for( auto & submodel : root->GetSubmodels() )
    {
        Model* foundNode = findMatchingNode( exclusion, submodel );
        if ( nullptr != foundNode ) return( foundNode );
    }

    // none of the subnodes matches the exclusion so let's try the current one
    bool foundParams = true;
    for( auto & term : exclusion )
    {
        foundParams = findParamInSubtree( term.first, root );
        if ( ! foundParams ) break;
    }
    if ( foundParams )
    {
        return( root );
    }
    else
    {
        return( nullptr );
    }
}

//
//
//
bool Task::findParamInSubtree( Parameter* parameter, Model* root )
{
    // check if exists in current node collection
    for( auto p : root->GetParameters() )
    {
        if ( p == parameter ) return( true );
    }

    // check if exists in any of the children
    for( auto & submodel : root->GetSubmodels() )
    {
        bool found = findParamInSubtree( parameter, submodel );
        if ( found ) return( true );
    }

    return( false );
}

//
//
//
void Task::deriveExclusions()
{
    ExclusionDeriver deriver(this);
    ParamCollection params;
    m_rootModel->GetAllParameters(params);

    for( auto p : params )
    {
        deriver.AddParameter( p );
    }

    for( auto & excl : m_exclusions )
    {
        deriver.AddExclusion( const_cast<Exclusion&>( excl ), true );
    }

    deriver.DeriveExclusions();
    
    // must clear and repopulate the collection with the result of the derivation
    m_exclusions.clear();
    __insert( m_exclusions, deriver.GetExclusions().begin(), deriver.GetExclusions().end() );
}

}
