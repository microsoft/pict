#include "generator.h"
using namespace std;

namespace pictcore
{

//
//
//
void Parameter::CleanUp()
{
    m_pending = false;
    m_bound   = false;

    m_combinations.clear();
    m_result.clear();
    m_exclusions.clear();
}


//
// This method binds the parameter to a specified value, updating the worklist
// Binding a value updates any combination zeros it completes as a side effect
// Binding a value also adds items to work list as a side effect
//
bool Parameter::Bind(int value, WorkList& worklist)
{
    DOUT(L"Binding " << m_name << L" to value " << value << L".\n");
    assert(!m_bound);
    assert(value < m_valueCount);

    m_result.push_back(value);
    m_currentValue = value;
    m_bound = true;

    for( auto & combination : m_combinations )
    {
        if( combination->AddBinding() == combination->GetParameterCount() - 1 )
        {
            // Add any parameter that completes a combination to work list
            for( int n = 0; n < combination->GetParameterCount(); ++n )
            {
                if( !( (*combination)[ n ] ).GetBoundCount() )
                {
                    worklist.AddItem( &( *combination )[ n ] );
                }
            }
        }
    }

    worklist.Print();
    
    return true;
}

//
// This method chooses the value for this parameter in the current context
//
int Parameter::PickValue()
{
    assert(!m_bound);

    int maxComplete    = 0;
    int maxTotal       = 0;
    int bestValue      = 0;
    int bestValueCount = 0;

    m_bound = true; // a white lie to facilitate Combination::Feasible()

    for (int value = 0; value < m_valueCount; ++value)
    {
        int totalZeros = 0;
        int complete   = 0;
        // try binding that value and see how feasible it is
        m_currentValue = value;

        for (ComboCollection::iterator iter = m_combinations.begin(); iter != m_combinations.end(); ++iter)
        {
            int zeros = (*iter)->Feasible();
            totalZeros += zeros;
            if ((*iter)->GetBoundCount() >= (*iter)->GetParameterCount() - 1)
            {
                if (zeros)
                    ++complete;

                if ((*iter)->ViolatesExclusion())
                {
                    // make sure we don't pick this value, and move on
                    totalZeros = -1;
                    complete   = -1;
                    break;
                }
            }
        }
        if (complete > maxComplete)
        {
            maxComplete    = complete;
            bestValue      = value;
            maxTotal       = totalZeros;
            bestValueCount = 1;
        }
        else if (complete == maxComplete)
        {
            if (totalZeros > maxTotal)
            {
                bestValue      = value;
                maxTotal       = totalZeros;
                bestValueCount = 1;
            }
            else if (!m_valueWeights.empty() && 0 == totalZeros && 0 == maxTotal)
            {
                // Arbitrary choice - we already have this parameter covered
                // Use weights if they exist
                bestValueCount += GetWeight(value);
                if (rand() % bestValueCount < GetWeight(value))
                {
                    bestValue = value;
                }
            }
            else if (totalZeros == maxTotal && !(rand() % ++bestValueCount))
            {
                bestValue = value;
                maxTotal  = totalZeros;
            }
        }
    }
    m_bound = false;

    // What if bestValueCount is 0 here due to exclusions?
    // That would be a bug, but better put in a check.
    if( m_task->GetGenerationMode() != GenerationMode::Preview )
    {
        assert( bestValueCount > 0 );
        if( bestValueCount <= 0 )
        {
            throw GenerationError( __FILE__, __LINE__, ErrorType::GenerationFailure );
        }
    }

    return bestValue;
}

//
//
//
void Parameter::SetWeights( vector<int> weights )
{
    assert( weights.size() == m_valueCount );
    m_valueWeights = weights;
}

//
// Return the parameter collection of the pseudoparameter's underlying Model
//
ParamCollection* PseudoParameter::GetComponents()
{
    return &m_model->GetParameters();
}

//
//
//
PseudoParameter::PseudoParameter( int order, unsigned int sequence, Model *model ) :
        Parameter( order, sequence, static_cast<int>( model->GetResultCount() ), wstring(), false ),
        m_model( model )
{
    m_name = L"";
    for( vector<Parameter*>::iterator ip =  model->GetParameters().begin();
                                      ip != model->GetParameters().end();
                                    ++ip )
    {
        if( ip != model->GetParameters().begin() )
        {
            m_name += L" ";
        }
        m_name += ( *ip )->GetName();
    }
}

}