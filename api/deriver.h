#pragma once

#include "generator.h"
#include "trie.h"

namespace pictcore
{

typedef std::list<Exclusion*> ExclPtrList;

//
//
//
class ExclusionDeriver
{
public:
    ExclusionDeriver( Task* Task ) : m_task( Task ) {}

    void AddParameter( Parameter* param ) { m_parameters.push_back( param ); }
    std::pair<ExclusionCollection::iterator, bool> AddExclusion( Exclusion& exclusion, bool checkIfInCollection );

    ParamCollection&     GetParameters() { return( m_parameters ); }
    ExclusionCollection& GetExclusions() { return( m_exclusions ); }

    void DeriveExclusions();

private:
    void buildExclusion( Exclusion &Excl, std::vector<ExclPtrList>::iterator begin );
    bool consistent( Exclusion &a, Exclusion &b );
    bool alreadyInCollection( Exclusion &a );

    void markDeleted ( Exclusion *excl ) { excl->markDeleted(); m_deletedAtLeastOne = true; }
    void markObsolete( ExclusionCollection::iterator ie );
    void peformDelete();

    void printLookup() { printLookupNode( m_lookup.get_root(), 0 ); }
    void printLookupNode( trienode<Exclusion::_ExclusionVec::value_type>* node, int indent );

    ParamCollection                    m_parameters;
    ExclusionCollection                m_exclusions;
    Parameter*                         m_currentParam;
    Task*                              m_task;
    std::vector<ExclPtrList>::iterator m_end;
    std::deque<Parameter* >            m_worklist;
    trie<Exclusion::_ExclusionVec>     m_lookup;

    bool m_deletedAtLeastOne;
};

}