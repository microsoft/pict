#include "deriver.h"
using namespace std;

namespace pictcore
{

//
// Consistent
// Predicate to check whether two exclusions are consistent
// They're not consistent if they have matching parameters with different values
// Ignore terms related to the current parameter, which we're covering
//
bool ExclusionDeriver::consistent( Exclusion &aa, Exclusion &bb )
{
    // make sure the smaller one is in the outer loop
    Exclusion *a;
    Exclusion *b;
    if( aa.size() < bb.size() )
    {
        a = &aa;
        b = &bb;
    }
    else
    {
        a = &bb;
        b = &aa;
    }

    Exclusion::iterator ib = b->begin();
    for( Exclusion::iterator ia = a->begin(); ia != a->end(); ++ia )
    {
        if( m_currentParam == ia->first )
            continue;

        // move ib to see if we can find ia's parameter
        // ia and ib are sorted by (i->first)->GetSequence() asc then i->second
        while( ib != b->end() && ( ia->first )->GetSequence() > ( ib->first )->GetSequence() )
            ++ib;

        // end of b, return true
        if( ib == b->end() )
            return true;

        // see if params match; if not, move on
        if( ( ia->first )->GetSequence() != ( ib->first )->GetSequence() )
            continue;

        // params match, what about values
        if( ia->second != ib->second )
            return false;
    }
    return true;
}

//
//
//
pair<ExclusionCollection::iterator, bool> ExclusionDeriver::AddExclusion( Exclusion& excl, bool checkIfInCollection )
{
    // if there's already an exclusion that's broader, do not add
    if( checkIfInCollection )
    {
        if( alreadyInCollection( excl ) )
        {
            return make_pair( m_exclusions.end(), false );
        }
    }

    // now add
    pair<ExclusionCollection::iterator, bool> result = m_exclusions.insert( excl );
    if( result.second )
    {
        // later, we will attempt to find sorted lists so 
        // the lookup structure should keep them sorted, too
        sort( excl.lbegin(), excl.lend() );
        m_lookup.insert( excl.GetList() );
    }

    return result;
}

//
// Marking exclusions as deleted. The reasoning behind this set of functions:
// When we add a new exclusion it may happen that it is broader than some
//    exclusions already in the collection which renders those obsolete.
// We mark them as deleted. Such exclusions will no longer participate in
//    the derivation process but will remain in the collection until it's
//    safe to remove them which is after any element from the m_worklist
//    is fully processed.
//

//
// given an exclusion mark those in the collection which are now obsolete
//
void ExclusionDeriver::markObsolete( ExclusionCollection::iterator ie )
{
    // empty exclusions would obsolete all others; we don't want to remove those
    if( ie->empty() ) return;

    for( ExclusionCollection::iterator im =  m_exclusions.begin();
                                       im != m_exclusions.end(); 
                                     ++im )
    {
        if( ( ie != im )
         && ( !im->isDeleted() )
         && contained( const_cast<Exclusion&> ( *ie ), const_cast<Exclusion&>( *im ) ) )
        {
            markDeleted( const_cast<Exclusion*>( &*im ) );
        }
    }
}

//
// Add a pointer to a given exclusion from each referenced parameter
//
void AddExclusionParamBackPtrs( ExclusionCollection::iterator& excl )
{
    for( Exclusion::iterator i_term = excl->begin(); i_term != excl->end(); ++i_term )
    {
        i_term->first->LinkExclusion( excl );
    }
}


//
// Clean the structure from the exclusions marked for deletion
//
void ExclusionDeriver::peformDelete()
{
    if( !m_deletedAtLeastOne ) return;

    //
    // todo:
    // This approach must be changed, we should be removing unnecessary links instead of recreating them
    // For now however, the recreation seems safer and works fast enough
    //

    // actually delete all marked as deleted
    ExclusionCollection::iterator ie = m_exclusions.begin();
    while( ie != m_exclusions.end() )
    {
        if( ie->isDeleted() )
        {
            Exclusion* pExcl = const_cast<Exclusion*>( &*ie );
            m_lookup.erase( pExcl->GetList() );
            ie = __map_erase( m_exclusions, ie );
        }
        else
            ++ie;
    }

    // recreate links to params
    for( Parameter* param : m_parameters )
    {
        param->ClearExclusions();
    }

    for( ie = m_exclusions.begin(); ie != m_exclusions.end(); ++ie )
    {
        AddExclusionParamBackPtrs( ie );
    }

    // prepare for the next round
    m_deletedAtLeastOne = false;
    DOUT( L"obsolete exclusions removed, now have: " << static_cast<int>( m_exclusions.size() ) << L"\n" );
}

//
// Recursion helper for ProcessExclusions, below
// Recursively builds all cover sets for given parameter
// Setup: ProcessExclusions has built a vector of vectors of exclusions
//        for this parameter, organized into buckets by value.
// Also, ProcessExclusions has set up static ptrs to model, parameter
//
inline void ExclusionDeriver::buildExclusion( Exclusion& exclImplied, vector<ExclPtrList>::iterator begin )
{
    // derivation can take a long time, use the call back to check if it should be aborted
    if( m_task->AbortGeneration() )
    {
        throw GenerationError( __FILE__, __LINE__, ErrorType::GenerationCancelled );
    }

    if( begin != m_end )
    {
        // for each exclusion in this bucket
        for( ExclPtrList::iterator i = begin->begin(); i != begin->end(); ++i )
        {
            // don't bother if the exclusion was already marked obsolete
            if( ( *i )->isDeleted() ) continue;

            // check for contradiction, continue if found
            if( !consistent( exclImplied, **i ) ) continue; 

            // merge the exclusion into the given one to make a new one
            Exclusion excl( exclImplied );
            for( Exclusion::iterator ie = ( *i )->begin(); ie != ( *i )->end(); ++ie )
            {
                if( ie->first != m_currentParam )
                {
                    excl.insert( *ie );
                }
            }

            // checking for being in the collection while still in progress of generation 
            // is just an optimization but it speeds things up considerably
            // The reasoning: if at this point the exclusion being built is already obsolete
            // we should stop building immediatelly and move on
            // however, it does not make sense to check if an exclusion is in the collection
            // before it reaches a size of 3; very rarely there will be an broader
            // exclusion for size <3

            if( alreadyInCollection( excl ) ) continue;

            // call buildExclusion on next bucket, passing our exclusion in progress
            buildExclusion( excl, begin + 1 );
        }
    }
    else
    {
        // add new exclusion to model
        //   first  = an iterator to the new exclusion,
        //   second = return whether item was added (true means it wasn't a duplicate)
        pair<ExclusionCollection::iterator, bool> addResult = AddExclusion( exclImplied, false );
        if( !addResult.second ) return;

        // now go through the collection and mark those that became obsolete
        markObsolete( addResult.first );

        DOUT( static_cast<int>( m_exclusions.size() ) << L" " );

        // add new references to existing structures
        AddExclusionParamBackPtrs( addResult.first );

        // if exclusion count >= value count for any params in new exclusion
        for( Exclusion::iterator i = exclImplied.begin(); i != exclImplied.end(); ++i )
        {
            if( i->first->GetExclusionCount() >= i->first->GetValueCount() &&
                m_worklist.end() == find( m_worklist.begin(), m_worklist.end(), i->first ) )
            {
                m_worklist.push_back( i->first );
                DOUT( L"worklist: 1 added\n" );
            }
        }
    }
}

//
// Given an exclusion, the function returns true if m_exclusions already has 
//    this or a more general exclusion
// This function makes a use of buckets to check only against those exclusions
//    in m_exclusions which have at least one element of the given one
//
inline bool ExclusionDeriver::alreadyInCollection( Exclusion &excl )
{
    // for all permutations
    sort( excl.lbegin(), excl.lend() );
    bool more = true;
    while( more )
    {
        if( m_lookup.find_prefix( excl.GetList() ) ) return true;
        more = next_permutation( excl.lbegin(), excl.lend() );
    }
    return false;
}

//
// Derive implicit exclusions and add to collection, adding to parameters also
//
// Pseudocode for deriving implicit exclusions:
// Set up a worklist (a collection of parameter pointers or references)
// Add every parameter with at least as many exclusions as values to worklist
// While there's a parameter in the worklist, get it
//    make a collection by value for this param of buckets of related exclusions
//    put ref to each exclusion in appropriate value-relative bucket
//    if all the buckets have something in them
//        recursively build all cover sets, building up exclusion on other params
//        back off if contradictory values encountered
//        check generated exclusion for redundancy, skip it if we've already got it
//        add new exclusion to model's exclusion collection
//        call ExclusionParamBackPtrs on new exclusion
//        if a new exclusion bumps exclusion count >= value count for any params
//            add to worklist
//
void ExclusionDeriver::DeriveExclusions()
{
    // For preview generation we don't invoke the core of the deriver at all
    if( m_task->GetGenerationMode() != GenerationMode::Regular ) return;

    DOUT( L"Exclusions:\n" );
    for( auto & exclusion : m_exclusions )
    {
        exclusion.Print();
    }
    DOUT( L"Derivation: Started\n" );

    // Add pointers to referencing exclusions to each parameter
    // for_each(m_exclusions.begin(), m_exclusions.end(), AddExclusionParamBackPtrs);
    for( ExclusionCollection::iterator ie = m_exclusions.begin(); ie != m_exclusions.end(); ++ie )
    {
        AddExclusionParamBackPtrs( ie );
    }

    // Initialize the worklist
    for( vector<Parameter*>::iterator i = m_parameters.begin(); i != m_parameters.end(); ++i )
    {
        if( ( *i )->GetExclusionCount() >= ( *i )->GetValueCount() )
        {
            m_worklist.push_back( &**i );
        }
    }

    while( !m_worklist.empty() )
    {
        DOUT( L"worklist size: " << static_cast<int>( m_worklist.size() ) << L"\n" );
        DOUT( L"excl col size: " << static_cast<int>( m_exclusions.size() ) << L"\n"; )

        m_currentParam = m_worklist[ 0 ];
        m_worklist.pop_front();

        DOUT( L"worklist item: [" << m_currentParam->GetName() << L"]" <<
              L", avg excl size: " << m_currentParam->GetAverageExclusionSize() <<
              L", num excls: " << m_currentParam->GetExclusionCount() <<
              L"\n"; )

        // We need buckets to hold the exclusions pertaining to each value
        vector<list<Exclusion *> > buckets( m_currentParam->GetValueCount() );

        for( ExclIterCollection::iterator ie =  m_currentParam->GetExclusions().begin();
                                          ie != m_currentParam->GetExclusions().end();
                                        ++ie )
        {
            // put ref to each exclusion in appropriate value-relative bucket
            // find this parameter in the exclusion
            Exclusion::iterator iExcl = find_if( ( *ie )->begin(), ( *ie )->end(),
                                                 [this](const ExclusionTerm et) {
                                                     return et.first == m_currentParam;
                                                 } );

            // put the exclusion in the right bucket
            assert( iExcl != ( *ie )->end() );
            Exclusion* pExcl = const_cast<Exclusion*>( &**ie );
            buckets.at( iExcl->second ).push_back( pExcl );
        }

        // if all the buckets have something in them
        int n;
        for( n = 0; n < m_currentParam->GetValueCount(); ++n )
        {
            if( buckets[ n ].empty() ) break;
        }

        // if there was an empty bucket, go to the next entry in the worklist
        if( n < m_currentParam->GetValueCount() ) continue;

        // recursively build all cover sets, building up exclusion on other params
        Exclusion exclImplied;
        m_end = buckets.end();
        buildExclusion( exclImplied, buckets.begin() );

        peformDelete();
    }

    DOUT( L"Derivation: Finished\n" );
    DOUT( L"Exclusions:\n" );
    for( auto & exclusion : m_exclusions )
    {
        exclusion.Print();
    }
}

//
//
//
void ExclusionDeriver::printLookupNode( trienode<Exclusion::_ExclusionVec::value_type> *node, int indent )
{
    for( auto ii = node->children.begin(); ii != node->children.end(); ++ii )
   {
        for( int i = 0; i < indent; ++i )
            DOUT( L"  " );
        DOUT( ii->first.first->GetName() << L": " << ii->first.second << L" [" << ii->second->value << L"]\n" );
        printLookupNode( ii->second, indent + 1 );
    }
}

}
