// ------------------------------------------------------------------------------
// Simplified trie; not thread-safe
// ------------------------------------------------------------------------------
#pragma once

#include <map>

//
//
//
template <class Item>
class trienode
{
public:
    typedef std::map< Item, trienode<Item>* > TNodes;

    TNodes children;
    bool   valid;

    trienode() : valid( false ) {}

    ~trienode()
    {
        for( typename TNodes::iterator ii = children.begin(); ii != children.end(); ++ii )
        {
            delete ii->second;
        }
    }
};

//
//
//
template <class Col>
class trie
{
    typedef typename trienode<typename Col::value_type>::TNodes TNodeCol;

public:
    //
    //
    //
    trie()
    {
        m_root = new trienode<typename Col::value_type>;
    }

    //
    //
    //
    ~trie()
    {
        delete m_root;
    }

    //
    //
    //
    trienode<typename Col::value_type>* get_root() { return m_root; }

    //
    // returns false if insert failed (OutOfMemory)
    //
    bool insert( Col &t )
    {
        trienode<typename Col::value_type>* current = m_root;
        for( typename Col::iterator ic = t.begin(); ic != t.end(); ++ic )
        {
            typename TNodeCol::iterator found;
            found = current->children.find( *ic );
            if( found == current->children.end() )
            {
                trienode< typename Col::value_type > *node = nullptr;

                try
                {
                    node = new trienode < typename Col::value_type >;
                }
                catch(const std::bad_alloc& )
                {
                    return( false );
                }

                std::pair<typename TNodeCol::iterator, bool> ret = current->children.insert( make_pair( *ic, node ) );
                if( !ret.second ) return( false );

                found = ret.first;
            }
            current = found->second;
        }
        current->valid = true;

        return( true );
    }

    //
    // trienodes are not deallocated when an element is erased
    // this is for perf reasons; memory usage is not that important
    // the whole structure will be torn down in d'tor
    //
    void erase( Col& t )
    {
        trienode<typename Col::value_type>* node = pfind( t );
        if( nullptr == node ) return;
        node->valid = false;
    }

    //
    //
    //
    bool find_prefix( Col &t )
    {
        return( pfind_prefix( t ) != nullptr );
    }

private:
    trienode<typename Col::value_type>* m_root;

    //
    //
    //
    trienode<typename Col::value_type>* pfind( Col &t )
    {
        trienode<typename Col::value_type> *current = m_root;
        for( typename Col::iterator ic = t.begin(); ic != t.end(); ++ic )
        {
            typename TNodeCol::iterator found = current->children.find( *ic );
            if( found == current->children.end() ) return( nullptr );

            current = found->second;
        }
        return( current->valid ? current : nullptr );
    }

    //
    //
    //
    trienode<typename Col::value_type>* pfind_prefix( Col &t )
    {
        trienode<typename Col::value_type> *current = m_root;
        for( typename Col::iterator ic = t.begin(); ic != t.end(); ++ic )
        {
            typename TNodeCol::iterator found = current->children.find( *ic );
            if( found == current->children.end() ) return( nullptr );

            current = found->second;

            if( current->valid ) return( current );
        }

        return( current->valid ? current : nullptr );
    }
};

