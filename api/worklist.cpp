#include "generator.h"
using namespace std;

namespace pictcore
{

//
//
//
void WorkList::AddItem( Parameter* param )
{
    if( !param->IsPending() )
    {
        assert( !param->GetBoundCount() );
        m_rep.push_back( param );
        param->MarkPending();
    }
}

//
//
//
Parameter *WorkList::GetItem()
{
    Parameter *param = m_rep.front();
    assert( !param->GetBoundCount() );
    m_rep.pop_front();
    return param;
}

//
//
//
void WorkList::Print()
{
    DOUT( L"Worklist: " );
    for( auto wl : m_rep )
    {
        DOUT( wl->GetName() << L", " );
    }
    DOUT( L"\n" );
}

}