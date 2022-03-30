#include "generator.h"
#include "deriver.h"
using namespace std;

namespace pictcore
{

//
// function object to sort parameters in decreasing combinatorial order
//
class GreaterThanByOrder {
public:
    bool operator()(const Parameter *p1, const Parameter *p2) { return p1->GetOrder() > p2->GetOrder(); }
};

//
// function object to sort parameters in increasing input sequence order
//
class LessThanBySequence {
public:
    bool operator()(const Parameter *p1, const Parameter *p2) { return p1->GetSequence() < p2->GetSequence(); }
};

//
// recursive helper routine for combination generation
//
void Model::choose( ParamCollection::iterator first,     // the iterators apply to current stage of recursion
                    ParamCollection::iterator last,
                    int                       order,     // number of items to choose in the range from first to last
                    int                       realOrder, // real order of this combination
                    Combination&              baseCombo,
                    ComboCollection&          vecCombo ) // where to put result vector
{
    assert( order >= 0 && order <= realOrder );
    assert( realOrder > 0 );

    if( 0 == order )
    {
        // add the current combination to the result vector
        Combination *combo = new Combination( this );
        combo->Assign( baseCombo );
        vecCombo.push_back( combo );

        // add the combination to all the parameters
        // set up its bit vector
        // initialize the zero count
        // add to the global zero count Combination::GlobalZero
        int size = 1;
        for( int n = 0; n < combo->GetParameterCount(); ++n )
        {
            // link all parameters in the combination to this combination
            ( *combo )[ n ].LinkCombination( combo );
            // determine size of bit vector
            size *= ( *combo )[ n ].GetValueCount();
        }

        // set size of combination's bit vector
        combo->SetMapSize( size );
        return;
    }

    // iterate over the possible first element in the combination
    // the three parts of the loop conditional are all necessary!
    // first part makes sure we have enough elements for the current combination
    // second part decrements the loop variable, then ensures that we don't emit
    //   subsets of higher-order combinations
    while( distance( first, last ) >= order && ( ( *( --last ) )->GetOrder() == order || order != realOrder ) )
    {
        // add the parameter to the combination
        baseCombo.PushParameter( *last );

        // recurse over remaining elements
        choose( first, last, order - 1, realOrder, baseCombo, vecCombo );
        baseCombo.PopParameter();
    }
}

//
// primary method for computing generalized combinatorial designs
//
// Pseudocode:
//    while (there are still zeros for some combinations)
//        initialize all parameters to unbound
//        initialize work list to empty
//        do we have unused seed data?
//            yes: bind those values, make sure combinations get marked, zero counts (including global) updated
//        while some parameters remain unbound
//            is the work list empty?
//                yes: pick a zero from combination with the most zeros, bind corresponding values
//                no: pull a parameter from the work list and pick a value
//
void Model::gcd( ComboCollection &vecCombo )
{
    if( m_parameters.empty() ) return;

    processExclusions( vecCombo );

    fixRowSeeds();

    int maxRange = -1;
    for( ComboCollection::iterator iterC = vecCombo.begin(); iterC != vecCombo.end(); ++iterC )
    {
        if( ( *iterC )->GetRange() > maxRange )
        {
            maxRange = ( *iterC )->GetRange();
        }
    }
    if( maxRange <= 0 ) return;

    GetTask()->AllocWorkbuf( maxRange );

    m_totalCombinations = GlobalZerosCount;

    // main loop: repeat until we've found all required parameter value combinations
    while( GlobalZerosCount > 0 )
    {
        DOUT( L"Main loop: GlobalZerosCount " << GlobalZerosCount << L"\n" );

        if( GetTask()->AbortGeneration() )
        {
            // tear down all the combinations
            for( ComboCollection::iterator i = vecCombo.begin(); i != vecCombo.end(); ++i )
            {
                delete *i;
            }

            throw GenerationError( __FILE__, __LINE__, ErrorType::GenerationCancelled );
        }

        // initialize all parameters to unbound, not on work list
        for( vector<Parameter*>::iterator iter = m_parameters.begin(); iter != m_parameters.end(); ++iter )
        {
            ( *iter )->InitBinding();
        }
        
        int unbound = static_cast<int>( m_parameters.size() );

        // init all combinations bound, counts to zero
        for( ComboCollection::iterator iterC = vecCombo.begin(); iterC != vecCombo.end(); ++iterC )
        {
            ( *iterC )->InitBinding();
        }
        
        // initialize work list to empty
        WorkList worklist;

        // do we have unused seed data?
        // if yes: bind those values, make sure combinations get marked, zero counts (including global) updated
        if( !m_rowSeeds.empty() )
        {
            RowSeed &rowSeed = m_rowSeeds.front();
            for( RowSeed::iterator ir = rowSeed.begin(); ir != rowSeed.end(); ++ir )
            {
                ( ir->first )->MarkPending();
            }
            for( RowSeed::iterator ir = rowSeed.begin(); ir != rowSeed.end(); ++ir )
            {
                // find the parameter in the collection
                Parameter* param = ir->first;
                vector<Parameter*>::iterator found = find( m_parameters.begin(), m_parameters.end(), param );
                assert( found != m_parameters.end() );
                if( found != m_parameters.end() )
                {
                    unbound -= param->Bind( ir->second, worklist );
                }
            }
            m_rowSeeds.pop_front();
        }

        for( int cidx = 0; cidx < static_cast<int>( vecCombo.size() ); ++cidx )
        {
            vecCombo[ cidx ]->Print();
        }

        // Approximate mode must be handled differently from the other modes
        if( m_task->GetGenerationMode() == GenerationMode::Approximate )
        {
            // it's a row not an exclusion but Exclusion is a perfect data structure for it
            typedef Exclusion CandidateRow;
            
            CandidateRow candidateRow;

            size_t attempt = 0;
            bool stop = false;
            while( 1 )
            {
                candidateRow = generateRandomRow();

                if( !rowViolatesExclusion( candidateRow ) )
                    break;

                if( ++attempt >= m_task->GetMaxRandomTries() )
                {
                    stop = true;
                    break;
                }
            };
            if( stop ) break; // out of "GlobalZerosCount > 0" loop

            for( CandidateRow::iterator ir = candidateRow.begin(); ir != candidateRow.end(); ++ir )
            {
                ( ir->first )->MarkPending();
            }
            for( CandidateRow::iterator ir = candidateRow.begin(); ir != candidateRow.end(); ++ir )
            {
                // find the parameter in the collection
                Parameter* param = ir->first;
                vector<Parameter*>::iterator found = find( m_parameters.begin(), m_parameters.end(), param );
                assert( found != m_parameters.end() );
                if( found != m_parameters.end() )
                {
                    unbound -= param->Bind( ir->second, worklist );
                }
            }
        }
        // non Approximate aka regular mode
        else
        {
            // loop to build one result vector: while some parameters remain unbound
            while( unbound > 0 )
            {
                if( worklist.IsEmpty() )
                {
                    DOUT( L"Empty worklist: finding a seed combination.\n" );
                    // pick a zero from feasible combination with the most zeros, bind corresponding values
                    int maxZeros = 0;
                    int maxMatch = 0;
                    int ties     = 0;
                    int choice   = -1;
                    for( int cidx = 0; cidx < static_cast<int>( vecCombo.size() ); ++cidx )
                    {
                        if( !vecCombo[ cidx ]->IsFullyBound() )
                        {
                            int open  = 0;
                            int match = 0;

                            // Make sure there's a feasible value set
                            for( int vidx = 0; vidx < vecCombo[ cidx ]->GetRange(); ++vidx )
                            {
                                ComboStatus status = vecCombo[ cidx ]->Feasible( vidx );
                                if( ComboStatus::Open == status )
                                {
                                    ++open;
                                }
                                else if( ComboStatus::CoveredMatch == status )
                                {
                                    ++match;
                                }
                            }

                            // if combo has the most zeros
                            if( open > maxZeros )
                            {
                                choice   = cidx;
                                ties     = 1;
                                maxZeros = open;
                            }
                            // if number of zeros ties up with some previous choice, pick randomly
                            else if( open == maxZeros
                                 &&  maxZeros > 0
                                 &&  !( rand() % ++ties ) )
                            {
                                choice = cidx;
                            }
                            // no zeros were found anywhere yet, pick the best matching one
                            else if( maxZeros == 0
                                 &&  match > maxMatch )
                            {
                                choice = cidx;
                                ties = 1;
                                maxMatch = match;
                            }
                            // if there's a tie in match, pick randomly
                            else if( maxZeros == 0
                                 &&  match > 0
                                 &&  match == maxMatch
                                 &&  !( rand() % ++ties ) )
                            {
                                choice = cidx;
                            }
                        }
                    } // for cidx
                    assert( choice != -1 );
                    if( -1 == choice )
                    {
                        throw GenerationError( __FILE__, __LINE__, ErrorType::GenerationFailure );
                    }

                    // no combo had any uncovered combinations in it
                    if( 0 == maxZeros )
                    {
                        int totalWeight = 0;
                        int bestValue   = -1;
                        // For each non-excluded value in the bitvec, find total weight
                        for( int vidx = 0; vidx < vecCombo[ choice ]->GetRange(); ++vidx )
                        {
                            if( ComboStatus::Excluded != vecCombo[ choice ]->Feasible( vidx ) )
                            {
                                // Pick a value using weighted random choice
                                int weight = vecCombo[ choice ]->Weight( vidx );
                                totalWeight += weight;
                                if( rand() % totalWeight < weight )
                                {
                                    bestValue = vidx;
                                }
                            }
                        }
                        unbound -= vecCombo[ choice ]->Bind( bestValue, worklist );
                    }
                    else
                    {
                        // OK, we picked a combination, now pick a value set
                        vector<int> candidates;
                        for( int vidx = 0; vidx < vecCombo[ choice ]->GetRange(); ++vidx )
                        {
                            if( ComboStatus::Open == vecCombo[ choice ]->Feasible( vidx ) )
                            {
                                candidates.push_back( vidx );
                            }
                        }
#if (0)
                        if (candidates.empty())
                        {
                            for (int vidx = 0; vidx < vecCombo[choice]->GetRange(); ++vidx)
                            {
                                if (CoveredMatch == vecCombo[choice]->Feasible(vidx))
                                {
                                    candidates.push_back(vidx);
                                }
                            }
                        }
#endif
                        assert( !candidates.empty() );
                        int zeroVal = candidates[ rand() % candidates.size() ];
                        DOUT( L"Chose value " << zeroVal << L", unbound count was " << unbound << L".\n" );
                        // Bind the values corresponding to the zero
                        unbound -= vecCombo[ choice ]->Bind( zeroVal, worklist );
                        DOUT( L"After combo bind, unbound count was " << unbound << L".\n" );
                    }
                }
                // worklist is NOT empty
                else
                {
                    // pull a parameter from the work list and pick a value
                    Parameter *param = worklist.GetItem();
                    assert( !param->GetBoundCount() );
                    DOUT( L"From worklist " );
                    param->Bind( param->PickValue(), worklist );
                    --unbound;
                }
            } // end: unbound > 0
        } // end: non-Approximate
    }

    m_remainingCombinations = GlobalZerosCount;

    // tear down all the combinations
    for( ComboCollection::iterator i = vecCombo.begin(); i != vecCombo.end(); ++i )
    {
        delete *i;
    }

    resolvePseudoParams();

    // put parameter vector back into original sequence
    sort( m_parameters.begin(), m_parameters.end(), LessThanBySequence() );

    // move results from parameters into this Model
    size_t results = 0;
    for( ParamCollection::iterator ip = m_parameters.begin(); ip != m_parameters.end(); ++ip )
    {
        ( *ip )->GetFirst();
        if( results )
        {
            assert( ( *ip )->GetTempResultCount() == results );
        }
        else
        {
            results = ( *ip )->GetTempResultCount();
        }
    }
    for( size_t n = 0; n < results; ++n )
    {
        ResultRow resultRow;
        for( ParamCollection::iterator ip = m_parameters.begin(); ip != m_parameters.end(); ++ip )
        {
            resultRow.push_back( ( *ip )->GetNext() );
        }

        // remove violating cases, it's needed for preview mode of
        //   generation as in that mode, m_result contains some invalid cases
        bool violatesExclusion = false;

        if( GetTask()->GetGenerationMode() == GenerationMode::Preview )
        {
            violatesExclusion = rowViolatesExclusion( resultRow );
        }
        if( !violatesExclusion )
        {
            m_results.push_back( resultRow );
        }
    }

    for( ParamCollection::iterator ip = m_parameters.begin(); ip != m_parameters.end(); ++ip )
    {
        ( *ip )->CleanUp();
    }

    // process all result parameters: mark unknown all result values
    //   that are under or over-specified.
    // only run it on the root; result params should not be part of
    //   any user-defined submodels anyway.
    if( this->GetTask()->GetRootModel() == this )
    {
        markUndefinedValuesInResultParams();
    }
}

//
// does a check on result params and makes sure only test cases with
// expected result defined explictly get to keep expected results
//
void Model::markUndefinedValuesInResultParams()
{
    // don't bother if no result parameters were defined
    if( GetResultParameterCount() <= 0 ) return;

    
    // set up an auxiliary structure that for each result parameter holds
    // an array of as many elements as there are rows in the output; each
    // element is a set of integers

    // for each result param
    // initilize a vector of N lists, where N = number of rows

    map< Parameter*, vector< set< int > > > excludedResultValues;

    vector< set< int > > emptyVector;
    emptyVector.resize( m_results.size() );
    for( ParamCollection::iterator ip = m_parameters.begin(); ip != m_parameters.end(); ++ip )
    {
        if( ( *ip )->IsExpectedResultParam() )
        {
            excludedResultValues.insert( make_pair( *ip, emptyVector ) );
        }
    }

    // remove excluded elements
    // for each result row (i_row)
    //   for each exclusion (i_excl)
    //     if non-result items of i_excl consistent with i_row
    //     for each result param of i_excl (i_param)
    //       remove value of i_param from corresponding set in structure

    int row_idx = 0;
    for( ResultCollection::iterator i_row = m_results.begin();
                                    i_row != m_results.end(); ++i_row, ++row_idx )
    {
        for( ExclusionCollection::iterator i_excl = m_exclusions.begin();
                                           i_excl != m_exclusions.end(); ++i_excl )
        {
            // only look at exclusions that have only one result param in them
            // if there are two or more, they will skew the true picture
            if( i_excl->ResultParamCount() > 1 ) continue;

            bool matches = true;
            for( Exclusion::iterator i_term = i_excl->begin();
                                     i_term != i_excl->end(); ++i_term )
            {
                // TODO: when result params are suported check modl/modl021.txt doesn't break here
                if( !i_term->first->IsExpectedResultParam()
                &&  static_cast<int>(i_row->at( i_term->first->GetSequence() )) != i_term->second )
                {
                    matches = false;
                    break;
                }
            }

            if( matches )
            {
                for( Exclusion::iterator i_term = i_excl->begin();
                                         i_term != i_excl->end(); ++i_term )
                {
                    if( i_term->first->IsExpectedResultParam() )
                    {
                        excludedResultValues[ i_term->first ][ row_idx ].insert( i_term->second );
                    }
                }
            }
        }
    }

    DOUT( L"-- result param map --\n" );
    for( auto ip = excludedResultValues.begin(); ip != excludedResultValues.end(); ++ip )
    {
        DOUT( ip->first->GetName() << ":\n" );
        for( auto iv = ip->second.begin(); iv != ip->second.end(); ++iv )
        {
            for( auto il = iv->begin(); il != iv->end(); ++il )
            {
                DOUT( *il << L" " );
            }
            DOUT( "\n" );
        }
    }

    //mark output rows as unknown

    //for each i_row in S
    //for each result param (i_param) in i_row
    //if i_param's set is empty
    //mark corresponding result value in the output as unknown

    for( map< Parameter*, vector< set< int > > >::iterator ip = excludedResultValues.begin();
                                                           ip != excludedResultValues.end();
                                                           ++ip )
    {
        int col = ( ip->first )->GetSequence();

        for( size_t row = 0; row < ip->second.size(); ++row )
        {
            // all values have to be excluded except for 1
            if( static_cast<int>(ip->second[ row ].size()) != ip->first->GetValueCount() - 1 )
            {
                m_results[ row ][ col ] = Parameter::UndefinedValue;
            }
        }
    }
}

//
// turn pseudoparameters back into real parameters
//
void Model::resolvePseudoParams()
{
    // translate pseudo-parameters into collections of actual parameters
    // perform value translation on the translated parameters
    // reverse iteration by index because I'm modifying the vector as I go
    for( size_t index = GetParameters().size(); index--; )
    {
        Parameter* param = GetParameters()[ index ];
        vector<Parameter*>* comps = param->GetComponents();
        if( comps )
        {
            // this parameter is really a pseudoparameter
            for( size_t nComp = 0; nComp < comps->size(); ++nComp )
            {
                // do we have this parameter already?
                // it's fine to skip it as we're running into an exact duplicate
                ParamCollection::iterator p;
                for( p = m_parameters.begin(); p != m_parameters.end(); ++p )
                {
                    if( *p == ( *comps )[ nComp ] ) break;
                }
                if( p != m_parameters.end() ) continue;

                // map values using results in param
                for( ParamResult::iterator ir = param->GetTempResults().begin(); ir != param->GetTempResults().end(); ++ir )
                {
                    size_t nMappedValue = param->GetModel()->GetResults()[ ( *ir ) ][ nComp ];
                    ( *comps )[ nComp ]->GetTempResults().push_back( nMappedValue );
                }
                // add to the model
                AddParameter( ( *comps )[ nComp ] );
            }
            // delete, erase, and otherwise destroy evidence of pseudoparameter
            delete param;
            GetParameters().erase( GetParameters().begin() + index );
        }
    }
}

//
//  creates a set of exclusions for parameters used in more than one submodel.
//  this is to allow the same parameter in more than one submodel. These new
//    exclusions will ensure we only combine result rows of two submodels when
//    all overlapping parameters have identical values.
//
//  returns true if any new exclusions were added.
//
//  looks at every result row of all pseudoparams and creates an exclusion
//    every time row has the same parameter but different value
//
//  for each pair of submodels (pseudoparams)
//      for each param p1 in s1
//          find corresponsing param p2 in s2
//          if p2 does not exist, continue
//          for each row r1 in s1
//              for each row r2 in s2
//                  if r1.p1 != r2.p2
//                      add r1, r2 to exlusions
//
bool Model::excludeConflictingParamValues()
{
    bool ret = false;
    // for each pair of submodels (pseudoparams)
    for( size_t pseudo1 = 0; pseudo1 < GetParameters().size(); ++pseudo1 )
    {
        Parameter* s1 = GetParameters()[ pseudo1 ];
        assert( s1 );
        if( nullptr == s1 || nullptr == s1->GetComponents() ) continue;

        for( size_t pseudo2 = pseudo1 + 1; pseudo2 < GetParameters().size(); ++pseudo2 )
        {
            Parameter* s2 = GetParameters()[ pseudo2 ];
            assert( s2 );
            if( nullptr == s2 || nullptr == s2->GetComponents() ) continue;

            // for each param p1 in s1
            ParamCollection::iterator p1;
            ParamCollection::iterator p2;
            for( p1 = s1->GetComponents()->begin(); p1 != s1->GetComponents()->end(); ++p1 )
            {
                // find corresponsing param p2 in s2 by name
                for( p2 = s2->GetComponents()->begin(); p2 != s2->GetComponents()->end(); ++p2 )
                {
                    // compare by name, names are unique
                    if( *p1 == *p2 ) break;
                }

                // if p2 does not exist, continue
                if( p2 == s2->GetComponents()->end() ) continue;

                int np1 = static_cast<int>( distance( s1->GetComponents()->begin(), p1 ) );
                int np2 = static_cast<int>( distance( s2->GetComponents()->begin(), p2 ) );

                // for each row r1 in s1
                //     for each row r2 in s2
                for( int v1 = 0; v1 < s1->GetValueCount(); ++v1 )
                {
                    for( int v2 = 0; v2 < s2->GetValueCount(); ++v2 )
                    {
                        // if r1.p1 != r2.p2
                        //     add r1, r2 to exlusions  
                        if( s1->GetModel()->GetResults()[ v1 ][ np1 ] !=
                            s2->GetModel()->GetResults()[ v2 ][ np2 ] )
                        {
                            Exclusion excl;
                            excl.insert( make_pair( s1, v1 ) );
                            excl.insert( make_pair( s2, v2 ) );
                            m_exclusions.insert( excl );

                            ret = true;
                        }
                    }
                }
            }
        }
    }

    DOUT( L"-- exclusions for submodels -- size: " << (int) m_exclusions.size() << endl );
    for( auto & exclusion : m_exclusions )
    {
        exclusion.Print();
    }
    DOUT( L"------------------------------ " << endl );

    return( ret );
}

//
// 
//
void Model::Generate()
{
    DOUT( L"Num of params " << (int) m_parameters.size() <<
          L", order: "      << m_order <<
          L", seed: "       << m_randomSeed << endl );
    for( ParamCollection::iterator ip = m_parameters.begin(); ip != m_parameters.end(); ++ip )
        DOUT( ( *ip )->GetName() << L", order: " << ( *ip )->GetOrder() << endl );

    switch( m_generationType )
    {
    case GenerationType::MixedOrder:
        generateMixedOrder();
        break;
    case GenerationType::FixedOrder:
        generateFixedOrder();
        break;
    case GenerationType::Full:
        generateFull();
        break;
    case GenerationType::Flat:
        generateFlat();
        break;
    case GenerationType::Random:
        generateRandom();
        break;
    }
}

//
// handles flat special generation
// gcd automatically deals with exclusions, column overlaps, etc.
//  (isn't that nice?)
// just call this after doing all the sub-models
//
void Model::GenerateVirtualRoot()
{
    DOUT( L"GenerateVirtualRoot: Begin" << endl );

    // no support for result rows in this generation mode
    assert( 0 == GetResultParameterCount() );

    GlobalZerosCount = 0;

    // this node is a virtual root node
    // muster all parameters out of the sub-models
    // use IDs to avoid adding any parameter twice
    set<wstring> paramIds;
    for( SubmodelCollection::iterator i = m_submodels.begin(); i != m_submodels.end(); ++i )
    {
        for( vector<Parameter*>::iterator ip = ( *i )->GetParameters().begin(); ip != ( *i )->GetParameters().end(); ++ip )
        {
            if( paramIds.end() == find( paramIds.begin(), paramIds.end(), ( *ip )->GetName() ) )
            {
                paramIds.insert( ( *ip )->GetName() );
                AddParameter( *ip );
            }
        }
    }
    
    // don't need this, so don't wait for it to go out of scope
    paramIds.clear();

    // create a combination for every sub-model
    // mark all satisfied except those corresponding to output rows in sub-models
    ComboCollection vecCombo;
    for( SubmodelCollection::iterator i = m_submodels.begin(); i != m_submodels.end(); ++i )
    {
        Combination *combo = new Combination( this );
        vecCombo.push_back( combo );
        int size = 1;
        for( ParamCollection::iterator ip = ( *i )->GetParameters().begin(); ip != ( *i )->GetParameters().end(); ++ip )
        {
            ( *ip )->LinkCombination( combo );
            combo->PushParameter( *ip );
            size *= ( *ip )->GetValueCount();
        }

        // init all combinations to satisfied, don't count any zeros
        combo->SetMapSize( size, COVERED );
        // for each result row
        for( int ridx = 0; ridx < ( *i )->GetResultCount(); ++ridx )
        {
            size_t index = 0;
            // calculate index to combination corresponding to this sub-model output row
            const ResultRow& results = ( *i )->GetResults()[ ridx ];
            int cidx = 0;
            for( ParamCollection::iterator ip = ( *i )->GetParameters().begin(); ip != ( *i )->GetParameters().end(); ++ip )
            {
                index *= ( *ip )->GetValueCount();
                index += results[ cidx++ ];
            }
            // make this index a target to generate
            combo->SetOpen( (int) index );
        }
    }

    gcd( vecCombo );
    DOUT( L"GenerateVirtualRoot: End << end " );
}

//
// the most generic of methods; supports mixed-order models
//
void Model::generateMixedOrder()
{
    GlobalZerosCount = 0;
    // turn each submodel's output into a PseudoParameter and add to model
    for( SubmodelCollection::iterator im = m_submodels.begin(); im != m_submodels.end(); ++im )
    {
        // make a pseudo-parameter from the submodel, add to this model
        // ignore first arg (order) - it'll be reset below
        Parameter *pseudo = new PseudoParameter( m_order, ++m_lastParamId, *im );
        AddParameter( pseudo );
    }

    // initialize combinations based on contents of m_parameters
    vector<Combination*> vecCombo;

    // sort parameter vector by order to generate parameter combinations
    sort( m_parameters.begin(), m_parameters.end(), GreaterThanByOrder() );

    bool r1 = mapExclusionsToPseudoParameters();
    bool r2 = excludeConflictingParamValues();
    // TODO: do we realy need to derive if the real exclusions were mapped?
    //       exclusions for conflicting param values don't conflict with one another
    if( r1 || r2 ) deriveSubmodelExclusions();

    mapRowSeedsToPseudoParameters();

    // for (each order, from highest to lowest)
    //   for every N-way combinations of these and higher-order parameters
    for( ParamCollection::iterator lastOrder = m_parameters.begin(); lastOrder != m_parameters.end(); /*increment in mid-loop*/ )
    {
        int order = ( *lastOrder )->GetOrder();
        // Get the range of each order
        while( lastOrder != m_parameters.end() && ( *lastOrder )->GetOrder() == order )
            ++lastOrder;
        Combination baseCombo( this );
        choose( m_parameters.begin(), lastOrder, order, order, baseCombo, vecCombo );
    }

    gcd( vecCombo );
}

//
// used for N-wise, although the method is more general and supports mixed-order models
//
void Model::generateFixedOrder()
{
    GlobalZerosCount = 0;
    
    // turn each submodel's output into a PseudoParameter and add to model
    for( SubmodelCollection::iterator im = m_submodels.begin(); im != m_submodels.end(); ++im )
    {
        // make a pseudo-parameter from the submodel, add to this model
        Parameter *pseudo = new PseudoParameter( m_order, ++m_lastParamId, *im );
        AddParameter( pseudo );
    }

    // initialize combinations based on contents of m_parameters
    vector<Combination*> vecCombo;

    for( ParamCollection::iterator ip = m_parameters.begin(); ip != m_parameters.end(); ++ip )
    {
        ( *ip )->SetOrder( m_order );
    }
    bool r1 = mapExclusionsToPseudoParameters();
    bool r2 = excludeConflictingParamValues();
    if( r1 || r2 ) deriveSubmodelExclusions();

    mapRowSeedsToPseudoParameters();

    Combination baseCombo( this );
    choose( m_parameters.begin(), m_parameters.end(), m_order, m_order, baseCombo, vecCombo );
    gcd( vecCombo );
}

//
// Used for full combinatorial cross-product
//
void Model::generateFull()
{
    // no support for result rows in this generation mode
    assert( 0 == GetResultParameterCount() );

    GlobalZerosCount = 0;
    // turn each submodel's output into a PseudoParameter and add to model
    for( SubmodelCollection::iterator im = m_submodels.begin(); im != m_submodels.end(); ++im )
    {
        // Make a pseudo-parameter from the submodel, add to this model
        // Ignore first arg (order) - it'll be reset below
        Parameter *pseudo = new PseudoParameter( 2, ++m_lastParamId, *im );
        AddParameter( pseudo );
    }

    // initialize combinations based on contents of m_parameters
    vector<Combination*> vecCombo;

    // set all parameters to order = number of parameters
    // (include the pseudo-parameters)
    // let's estimate how many rows we'll produce
    long rows = 1;
    for( ParamCollection::iterator ip = m_parameters.begin(); ip != m_parameters.end(); ++ip )
    {
        ( *ip )->SetOrder( static_cast<int>( m_parameters.size() ) );
        rows *= ( *ip )->GetValueCount();
        if( rows > MaxRowsToGenerate )
        {
            throw GenerationError( __FILE__, __LINE__, ErrorType::TooManyRows );
        }
    }
    bool r1 = mapExclusionsToPseudoParameters();
    bool r2 = excludeConflictingParamValues();
    if( r1 || r2 ) deriveSubmodelExclusions();

    mapRowSeedsToPseudoParameters();

    Combination baseCombo( this );
    choose( m_parameters.begin(),
            m_parameters.end(),
            static_cast<int>( m_parameters.size() ),
            static_cast<int>( m_parameters.size() ),
            baseCombo, vecCombo );
    gcd( vecCombo );
}

//
// Random combinatorial model
//
void Model::generateRandom()
{
    // no support for result rows in this generation mode
    assert( 0 == GetResultParameterCount() );

    GlobalZerosCount = 0;
    // turn each submodel's output into a PseudoParameter and add to model
    for( SubmodelCollection::iterator im = m_submodels.begin(); im != m_submodels.end(); ++im )
    {
        // make a pseudo-parameter from the submodel, add to this model
        Parameter *pseudo = new PseudoParameter( 1, ++m_lastParamId, *im );
        AddParameter( pseudo );
    }

    // Initialize combinations based on contents of m_parameters
    vector<Combination*> vecCombo;
    for( vector<Parameter*>::iterator ip = m_parameters.begin(); ip != m_parameters.end(); ++ip )
    {
        ( *ip )->SetOrder( 1 );
    }
    bool r1 = mapExclusionsToPseudoParameters();
    bool r2 = excludeConflictingParamValues();
    if( r1 || r2 ) deriveSubmodelExclusions();

    mapRowSeedsToPseudoParameters();

    Combination baseCombo( this );
    choose( m_parameters.begin(), m_parameters.end(), 1, 1, baseCombo, vecCombo );
    gcd( vecCombo );
    if( m_maxRows > 0 && m_maxRows < static_cast<long>( m_results.size() ) )
    {
        m_results.erase( m_results.begin() + m_maxRows, m_results.end() );
    }
}

//
// Flat combinatorial model
//
void Model::generateFlat()
{
    // no support for result rows in this generation mode
    assert( 0 == GetResultParameterCount() );
    // no user-defined seeds allowed in this generation mode
    assert( m_rowSeeds.empty() );

    // seeds will ensure values in test cases proceed in the same order as in parameter definitions

    int maxValues = 0;
    for( vector<Parameter*>::iterator ip = m_parameters.begin(); ip != m_parameters.end(); ++ip )
    {
        maxValues = max( maxValues, static_cast<int>( ( *ip )->GetValueCount() ) );
    }

    int curVal = 0;
    while( curVal < maxValues )
    {
        RowSeed seed;

        for( vector<Parameter*>::iterator ip = m_parameters.begin(); ip != m_parameters.end(); ++ip )
        {
            if( curVal < ( *ip )->GetValueCount() )
            {
                seed.insert( make_pair( *ip, curVal ) );
            }
        }

        m_rowSeeds.push_back( seed );
        ++curVal;
    };

    // order is always 1 across all params
    m_order = 1;
    generateFixedOrder();

    // trim the output if necessary
    if( m_maxRows > 0 && m_maxRows < static_cast<long>( m_results.size() ) )
    {
        m_results.erase( m_results.begin() + m_maxRows, m_results.end() );
    }
}

//
//
//
Exclusion Model::generateRandomRow()
{
    Exclusion row;
    for( ParamCollection::iterator ip = m_parameters.begin(); ip != m_parameters.end(); ++ip )
    {
        int sum = 0;
        for( int i = 0; i < ( *ip )->GetValueCount(); ++i )
        {
            sum += ( *ip )->GetWeight( i );
        }
        int idx = rand() % sum;

        int n;
        int val = 0;
        for( n = 0; n < ( *ip )->GetValueCount(); ++n )
        {
            if( val < idx )
            {
                val += ( *ip )->GetWeight( n );
            }
            else
            {
                break;
            }
        }

        row.insert( make_pair( *ip, n ) );
    }
    return( row );
}


//
// creates a set of exclusions for pseudoparameters.
// returns true if any exclusions were added
//
bool Model::mapExclusionsToPseudoParameters()
{
    bool ret = false;

    // for each pseudoparameter
    for( size_t index = 0; index < GetParameters().size(); ++index )
    {
        Parameter* param = GetParameters()[ index ];
        ParamCollection* comps = param->GetComponents();
        
        if( comps )
        {
            // orepare a temporary collection for all exclusions being added
            ExclusionCollection newExcls;
        
            // this parameter is really a pseudoparameter
            for( ExclusionCollection::iterator iexcls = m_exclusions.begin(); iexcls != m_exclusions.end(); )
            {
                // partition exclusion terms into related, unrelated (to pseudoparam)
                Exclusion relatedTerms;
                Exclusion unrelatedTerms;
                for( Exclusion::iterator iexcl = iexcls->begin(); iexcl != iexcls->end(); ++iexcl )
                {
                    if( find( comps->begin(), comps->end(), iexcl->first ) == comps->end() )
                    {
                        unrelatedTerms.insert( *iexcl );
                    }
                    else
                    {
                        relatedTerms.insert( *iexcl );
                    }
                }

                // if any of exclusion's terms relate to underlying parameters
                if( !relatedTerms.empty() )
                {
                    // Get rid of the exclusion, since we'll replace it
                    m_exclusions.erase( iexcls++ );

                    // for each value of the pseudoparameter
                    for( int vidx = 0; vidx < param->GetValueCount(); ++vidx )
                    {
                        Exclusion::iterator irel;
                        for( irel = relatedTerms.begin(); irel != relatedTerms.end(); ++irel )
                        {
                            vector<Parameter*>::iterator ip = find( comps->begin(), comps->end(), irel->first );
                            if( comps->end() == ip )
                            {
                                break;
                            }
                            int nParam = static_cast<int>( distance( comps->begin(), ip ) );
                            int nRealVal = static_cast<int>(param->GetModel()->GetResults()[ vidx ][ nParam ]);
                            if( nRealVal != irel->second )
                            {
                                break;
                            }
                        }

                        // if value is excluded in overlapping underlying params
                        if( relatedTerms.end() == irel )
                        {
                            // generate a new exclusion using unrelated + this pseudo/value
                            Exclusion newExclusion( unrelatedTerms );
                            newExclusion.insert( make_pair( param, vidx ) );
                            // Put it into a temporary collection so we don't hit it again on this iteration
                            newExcls.insert( newExclusion );
                            ret = true;
                        }
                    }
                }
                else
                {
                    // no new exclusion, just increment loop iterator
                    ++iexcls;
                }
            }

            // copy all new exclusions to the main collection
            __insert( m_exclusions, newExcls.begin(), newExcls.end() );
        }
    }

    return( ret );
}

//
// there's a whole new set of exclusions created for pseudoparameters
// deriver must run on them
//
void Model::deriveSubmodelExclusions()
{
    ExclusionDeriver deriver( GetTask() );

    // the deriver must know about pseudo parameters
    ParamCollection params;
    GetAllParameters( params );
    for( ParamCollection::iterator ip = params.begin(); ip != params.end(); ++ip )
    {
        deriver.AddParameter( *ip );
    }

    // run the deriver on the newly created set of exclusions
    if( !deriver.GetParameters().empty() )
    {
        for( ExclusionCollection::iterator i_e = m_exclusions.begin(); i_e != m_exclusions.end(); ++i_e )
        {
            deriver.AddExclusion( const_cast<Exclusion&> ( *i_e ), true );
        }
        deriver.DeriveExclusions();

        // must clear because some initial exclusions may have been removed during derivation as obsolete
        m_exclusions.clear();
        __insert( m_exclusions, deriver.GetExclusions().begin(), deriver.GetExclusions().end() );
    }
}

//
// mark combinations, or create new ones, to represent exclusions
//
// pseudocode for applying exclusions:
// for each exclusion
//    use set intersection to get target combinations
//    if set is empty, create a combination matching the exclusion and add to set
//    for each target combination
//        recurse through parameters
//            if bound in exclusion, use exclusion value
//            else iterate over all values
//            terminate recursion: set result value excluded, update count
//
void Model::processExclusions( ComboCollection& vecCombo )
{
    // Sort every parameter's collection of combinations to prepare for set intersection
    for( vector<Parameter*>::iterator ip = m_parameters.begin(); ip != m_parameters.end(); ++ip )
    {
        ( *ip )->SortCombinations();
    }

    for( ExclusionCollection::iterator iexcl = m_exclusions.begin(); iexcl != m_exclusions.end(); ++iexcl )
    {
        // paranoia; shouldn't happen
        assert( !iexcl->empty() );
        if( iexcl->empty() ) continue;

        // form the intersection of the combinations associated with each of the
        //  parameters that make up this exclusion
        ComboCollection intersection;
        Exclusion::iterator iexclterm = iexcl->begin();
        Parameter* sourceParam = iexclterm->first;
        ComboCollection::const_iterator posCandidate = sourceParam->GetCombinationBegin();
        while( true )
        {
            // We're going around and around...
            if( iexcl->end() == ++iexclterm )
            {
                iexclterm = iexcl->begin();
            }

            if( iexclterm->first == sourceParam )
            {
                intersection.push_back( *posCandidate );
                if( ++posCandidate == sourceParam->GetCombinationEnd() ) break;
            }
            // find next >= in this param
            ComboCollection::const_iterator posLB = lower_bound( iexclterm->first->GetCombinationBegin(),
                                                                 iexclterm->first->GetCombinationEnd(),
                                                                 *posCandidate, CombinationPtrSortPred() );
            if( iexclterm->first->GetCombinationEnd() == posLB ) break;
            
            // if >, reset candidate and source
            if( *posLB != *posCandidate )
            {
                posCandidate = posLB;
                sourceParam = iexclterm->first;
            }
        }
        // If the result is empty, make a new combination from the exclusion,
        // mark all of its members satisfied, and make sure all the parameters in the new
        // combination still have sorted combination collections.
        if( intersection.empty() )
        {
            Combination *pNewCombo = new Combination( this );
            int size = 1;
            for( Exclusion::iterator ix = iexcl->begin(); ix != iexcl->end(); ++ix )
            {
                pNewCombo->PushParameter( ( ix->first ) );
                ix->first->LinkCombination( pNewCombo );
                ix->first->SortCombinations();
                size *= ix->first->GetValueCount();
            }
            // Set size of combination's bit vector
            pNewCombo->SetMapSize( size, COVERED );
            vecCombo.push_back( pNewCombo );
            intersection.push_back( pNewCombo );
        }

        for( ComboCollection::iterator ic = intersection.begin(); ic != intersection.end(); ++ic )
        {
            // all exclusions must be applied to the combo and not just the current one
            for( ExclusionCollection::iterator ie = m_exclusions.begin(); ie != m_exclusions.end(); ++ie )
            {
                ( *ic )->ApplyExclusion( const_cast<Exclusion&> ( *ie ) );
            }
        }
    }
}

//
// checks whether a given row violates any exclusions
// this function will be invoked only during preview generation
//
bool Model::rowViolatesExclusion( ResultRow& row )
{
    for( ExclusionCollection::iterator ie = m_exclusions.begin(); ie != m_exclusions.end(); ++ie )
    {
        bool matches = true;
        for( Exclusion::iterator it = ie->begin(); it != ie->end(); ++it )
        {
            if( static_cast<int>(row[ it->first->GetSequence() ]) != it->second )
            {
                matches = false;
                break;
            }
        }
        if( matches ) return( true );
    }
    return( false );
}

//
// the same function but row defined as an Exclusion
//
bool Model::rowViolatesExclusion( Exclusion& row )
{
    for( ExclusionCollection::iterator ie = m_exclusions.begin(); ie != m_exclusions.end(); ++ie )
    {
        if( contained( const_cast<Exclusion&> ( *ie ), row ) )
        {
            return( true );
        }
    }
    return( false );
}

//
// translates seeds of underlying parameters to seeds of this pseudoparam.
// this is almost the same procedure as MapExclusionsToPseudoParameters.
// the only difference is lack of exclusion deriver.
//
void Model::mapRowSeedsToPseudoParameters()
{
    // for each pseudoparameter
    for( size_t index = 0; index < GetParameters().size(); ++index )
    {
        Parameter* param = GetParameters()[ index ];
        vector<Parameter*>* comps = param->GetComponents();
        if( comps )
        {
            // Prepare a temporary collection for all seeds being added
            RowSeedCollection newSeeds;
            // This parameter is really a pseudoparameter
            for( RowSeedCollection::iterator iseeds = m_rowSeeds.begin(); iseeds != m_rowSeeds.end(); )
            {
                // partition the seed terms into related, unrelated (to pseudoparam)
                RowSeed relatedTerms, unrelatedTerms;
                for( RowSeed::iterator iseed = iseeds->begin(); iseed != iseeds->end(); ++iseed )
                {
                    if( find( comps->begin(), comps->end(), iseed->first ) == comps->end() )
                    {
                        unrelatedTerms.insert( *iseed );
                    }
                    else
                    {
                        relatedTerms.insert( *iseed );
                    }
                }
                // if any of seed's terms relate to underlying parameters
                if( !relatedTerms.empty() )
                {
                    // Get rid of the seed, since we'll replace it
                    m_rowSeeds.erase( iseeds++ );
                    // for each value of the pseudoparameter
                    for( int vidx = 0; vidx < param->GetValueCount(); ++vidx )
                    {
                        RowSeed::iterator irel;
                        for( irel = relatedTerms.begin(); irel != relatedTerms.end(); ++irel )
                        {
                            vector<Parameter*>::iterator ip = find( comps->begin(), comps->end(), irel->first );
                            if( comps->end() == ip )
                            {
                                break;
                            }
                            int nParam = static_cast<int>( distance( comps->begin(), ip ) );
                            int nRealVal = static_cast<int>(param->GetModel()->GetResults()[ vidx ][ nParam ]);
                            if( nRealVal != irel->second )
                            {
                                break;
                            }
                        }
                        // if value is excluded in overlapping underlying params
                        if( relatedTerms.end() == irel )
                        {
                            // generate a new seed using unrelated + this pseudo/value
                            RowSeed newSeed( unrelatedTerms );
                            newSeed.insert( make_pair( param, vidx ) );
                            // Put it into a temporary collection so we don't hit it again on this iteration
                            newSeeds.push_back( newSeed );
                        }
                    }
                }
                else
                {
                    // no new seed, just increment loop iterator
                    ++iseeds;
                }
            }

            // copy all new seeds to the main collection
            __push_back( m_rowSeeds, newSeeds.begin(), newSeeds.end() );
        }
    }
}

//
//
//
bool seedContained( RowSeed &container, RowSeed &containee )
{
    // containee has to have all its elements matching exacly some element in the container
    for( RowSeed::iterator ic = containee.begin(); ic != containee.end(); ++ic )
    {
        if( container.find( *ic ) == container.end() ) return( false );
    }
    return( true );
}

//
//
//
bool seedViolatesExclusion( RowSeed &seed, Exclusion &exclusion )
{
    // exclusion has to have all its elements matching exacly some element in the seed
    for( Exclusion::iterator ie = exclusion.begin(); ie != exclusion.end(); ++ie )
    {
        // translate exclusion item into seed item
        // currently they are the same thing
        RowSeedTerm term = make_pair( ie->first, ie->second );
        if( seed.find( term ) == seed.end() ) return( false );
    }
    return( true );
}

//
// remove obsolete seeds: seeds contained within other seeds
// remove seeds that violate exclusions
//
void Model::fixRowSeeds()
{
    DOUT( L"Before row seeds initial clean up.  Row seeds number: " << static_cast<int>( m_rowSeeds.size() ) << L"\n" );
    for_each( m_rowSeeds.begin(), m_rowSeeds.end(), printRowSeed );

    // remove parameters that don't belong to this model
    for( RowSeedCollection::iterator i_seeds = m_rowSeeds.begin(); i_seeds != m_rowSeeds.end(); ++i_seeds )
    {
        RowSeed::iterator i_seed = i_seeds->begin();
        while( i_seed != i_seeds->end() )
        {
            // find the parameter in the collection, verify the value is within the range
            // if something is not right, remove the seed item
            vector<Parameter*>::iterator param = find( m_parameters.begin(), m_parameters.end(), i_seed->first );

            if( param != m_parameters.end()
            &&  i_seed->second >= 0
            &&  i_seed->second < ( *param )->GetValueCount() )
            {
                ++i_seed;
            }
            else
            {
                i_seed = __map_erase( *i_seeds, i_seed );
            }
        }
    }

    DOUT( L"Before contained row seeds removed. Row seeds number: " << static_cast<int>( m_rowSeeds.size() ) << L"\n" );
    for_each( m_rowSeeds.begin(), m_rowSeeds.end(), printRowSeed );

    // eliminate all seeds contained within other seeds
    RowSeedCollection::iterator first = m_rowSeeds.begin();
    while( first != m_rowSeeds.end() )
    {
        if( first->empty() )
        {
            first = m_rowSeeds.erase( first );
        }
        else
        {
            RowSeedCollection::iterator second = m_rowSeeds.begin();
            while( second != m_rowSeeds.end() )
            {
                if( first == second || !seedContained( *first, *second ) )
                    ++second;
                else
                    second = m_rowSeeds.erase( second );
            }
            ++first;
        }
    }

    DOUT( L"After contained row seeds removed.  Row seeds number: " << static_cast<int>( m_rowSeeds.size() ) << L"\n" );
    for_each( m_rowSeeds.begin(), m_rowSeeds.end(), printRowSeed );

    // check if any seed violates any exclusion
    for( ExclusionCollection::iterator ie = m_exclusions.begin(); ie != m_exclusions.end(); ++ie )
    {
        RowSeedCollection::iterator is = m_rowSeeds.begin();
        while( is != m_rowSeeds.end() )
        {
            if( seedViolatesExclusion( *is, const_cast<Exclusion&>( *ie ) ) )
            {
                is = m_rowSeeds.erase( is );
            }
            else
            {
                ++is;
            }
        }
    }

    DOUT( L"After excl violating seeds removed. Row seeds number: " << static_cast<int>( m_rowSeeds.size() ) << L"\n" );
    for_each( m_rowSeeds.begin(), m_rowSeeds.end(), printRowSeed );
}

//
//
//
void printRowSeed( RowSeed &seed )
{
    for( RowSeed::iterator is = seed.begin(); is != seed.end(); is++ )
        DOUT( ( is->first )->GetName() << L": " << is->second << L" " );
    DOUT( L"\n" );
}

//
// don't clean up parameters here - they may be shared between models
//
Model::~Model()
{
    // clean up child models
    for( SubmodelCollection::iterator i = m_submodels.begin(); i != m_submodels.end(); ++i )
    {
        delete *i;
    }
}

}
