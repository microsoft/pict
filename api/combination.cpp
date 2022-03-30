#include "generator.h"
#include <cstring>

namespace pictcore
{

unsigned int Combination::m_lastUsedId = UNDEFINED_ID;

//
//
//
void Combination::SetMapSize( int size, TrackType trackType )
{
    m_range = size;
    m_bitvec = new TrackType[ size ];

    memset( m_bitvec, trackType, size * sizeof( TrackType ) );
    if( OPEN == trackType )
    {
        m_openCount = size;
        m_model->GlobalZerosCount += size;
    }
}

//
// Binds all associated parameters according to a key value
// The computation of parameter values from key is the inverse of Complete below
// Returns the number of parameters we bound
//
int Combination::Bind( int value, WorkList &worklist )
{
    int bound = 0;
    // Mark all as pending so we don't add any to the worklist
    for( auto param : m_params )
    {
        param->MarkPending();
    }

    for( ParamCollection::reverse_iterator iter =  m_params.rbegin();
                                           iter != m_params.rend();
                                         ++iter )
    {
        if( !( *iter )->GetBoundCount() )
        {
            ( *iter )->Bind( value % ( *iter )->GetValueCount(), worklist );
            ++bound;
        }
        value /= ( *iter )->GetValueCount();
    }

    return bound;
}

//
// Count the zeros in the projection of the current set of bound values
//
int Combination::Feasible()
{
    int* workbuf = m_model->GetTask()->GetWorkbuf();

    int nWorkVals = 1;
    workbuf[ 0 ] = 0;
    for( Parameter* param : m_params )
    {
        for( int i = 0; i < nWorkVals; i++ )
        {
            workbuf[ i ] *= param->GetValueCount();
        }
        int nCurrentVal = (int) param->GetLast();
        if( param->GetBoundCount() )
        {
            for( int i = 0; i < nWorkVals; ++i )
            {
                workbuf[ i ] += nCurrentVal;
            }
        }
        else
        {
            for( int i = 0; i < nWorkVals; ++i )
            {
                for( int addend = 1; addend < param->GetValueCount(); ++addend )
                {
                    workbuf[ i + nWorkVals*addend ] = workbuf[ i ] + addend;
                }
            }
            nWorkVals *= param->GetValueCount();
        }
    }

    // Now count the zeros corresponding to the open combinations
    int zeros = 0;
    for( int i = 0; i < nWorkVals; ++i )
    {
        if( ComboStatus::Open == m_bitvec[ workbuf[ i ] ] )
        {
            ++zeros;
        }
    }

    return zeros;
}

//
// See if the key value matches bound values
//
ComboStatus Combination::Feasible( int value )
{
    if( EXCLUDED == m_bitvec[ value ] )
    {
        return ComboStatus::Excluded;
    }
    ComboStatus retval = ( COVERED == m_bitvec[ value ] ) ? ComboStatus::CoveredMatch : ComboStatus::Open;
    for( ParamCollection::reverse_iterator iter = m_params.rbegin(); iter != m_params.rend(); ++iter )
    {
        if( ( *iter )->GetBoundCount() && 
            static_cast<int>(( *iter )->GetLast()) != value % ( *iter )->GetValueCount() )
        {
            return ComboStatus::Excluded;
        }
        value /= ( *iter )->GetValueCount();
    }

    return retval;
}

//
//
//
int Combination::Weight( int value )
{
    int weight = 0;
    for( ParamCollection::reverse_iterator iter = m_params.rbegin(); iter != m_params.rend(); ++iter )
    {
        Parameter* param = *iter;
        weight += param->GetWeight( value % param->GetValueCount() );
        value /= param->GetValueCount();
    }

    return weight;
}

//
//
//
int Combination::AddBinding()
{
    if( ++m_boundCount == static_cast<int>(m_params.size()) )
    {
        // compute which zero we're setting
        size_t value = 0;
        for( ParamCollection::iterator iter = m_params.begin(); iter != m_params.end(); ++iter )
        {
            assert( ( *iter )->GetBoundCount() );
            value = ( value * ( *iter )->GetValueCount() ) + ( *iter )->GetLast();
        }

        // check to see if it's zero
        if( OPEN == m_bitvec[ value ] )
        {
            // if so, set it and count it locally and globally
            assert( value <= (size_t) m_range );
            m_bitvec[ value ] = COVERED;
            --m_openCount;
            --m_model->GlobalZerosCount;
        }
    }

    return m_boundCount;
}

//
// Recurse through parameters
// If bound in exclusion, use exclusion value
//  else iterate over all values
// Terminate recursion: set result value excluded, update count
//
void Combination::applyExclusion( Exclusion& excl, int index, ParamCollection::iterator pos )
{
    if( m_params.end() == pos )
    {
        assert( index <= m_range );
        if( OPEN == m_bitvec[ index ] )
        {
            --m_openCount;
            --m_model->GlobalZerosCount;
        }
        m_bitvec[ index ] = EXCLUDED;
    }
    else
    {
        // Is *pos a parameter in the exclusion?
        Exclusion::iterator ie = find_if( excl.begin(),
                                          excl.end(),
                                          [pos](const ExclusionTerm et) {
                                              return et.first == *pos;
                                          } );
        // parameter is bound to a value in the exclusion
        if( excl.end() != ie )
        {
            applyExclusion( excl, index * ( *pos )->GetValueCount() + ie->second, pos + 1 );
        }
        // parameter ranges over all values in the exclusion
        else
        {
            for( int n = 0; n < ( *pos )->GetValueCount(); ++n )
            {
                applyExclusion( excl, index * ( *pos )->GetValueCount() + n, pos + 1 );
            }
        }
    }
}

//
//
//
void Combination::ApplyExclusion( Exclusion& excl )
{
    // only if this exclusion's parameters exist in the combo
    for( Exclusion::iterator it = excl.begin(); it != excl.end(); ++it )
    {
        ParamCollection::iterator ip = find( m_params.begin(), m_params.end(), it->first );
        if( ip == m_params.end() ) return;
    }

    applyExclusion( excl, 0, m_params.begin() );
}

//
//
//
bool Combination::ViolatesExclusion()
{
    size_t nKey = 0;
    for( ParamCollection::iterator iter = m_params.begin(); iter != m_params.end(); ++iter )
    {
        nKey *= ( *iter )->GetValueCount();
        size_t nCurrentVal = ( *iter )->GetLast();
        assert( ( *iter )->GetBoundCount() );
        nKey += nCurrentVal;
    }

    return ( EXCLUDED == m_bitvec[ nKey ] );
}

//
// Make a specific combination a target
// Fix up # of zeros in combination
// Fix up global zeros
//
void Combination::SetOpen( int index )
{
    assert( index < m_range );
    if( OPEN != m_bitvec[ index ] )
    {
        m_bitvec[ index ] = OPEN;
        ++m_openCount;
        ++m_model->GlobalZerosCount;
    }
}

//
//
//
Combination::Combination( Model *M ) :
    m_bitvec( nullptr ), m_range( 0 ), m_openCount( 0 ), m_boundCount( 0 ), m_model( M )
{
    m_id = ++m_lastUsedId;
    DOUT( L"Combination created: " << m_id << endl );
}

//
//
//
Combination::~Combination()
{
    DOUT( L"Combination deleted: " << m_id << endl );
    if( nullptr != m_bitvec ) delete[] m_bitvec;
}

//
//
//
void Combination::Print()
{
    DOUT( L"Combo dump: name " << GetId() << L" range " << m_range << L", " << m_openCount << L" zeros, "
          << ( m_boundCount ? L"" : L"not " ) << L"bound. " );
    DOUT( L" " << GetParameterCount() << L" parameters: " );
    for( int n = 0; n < GetParameterCount(); ++n )
        DOUT( ( *this )[ n ].GetName() << L" [" << ( *this )[ n ].GetSequence() << L"] " );
    DOUT( L" " );
    for( int n = 0; n < m_range; ++n )
        DOUT( m_bitvec[ n ] << L" " );
    DOUT( L"\n" );
}

}
