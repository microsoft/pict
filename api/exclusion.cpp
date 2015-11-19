#include "generator.h"

namespace pictcore
{
    
//
// this is to verify whether a is contained within b.
// if it's true, b is obsolete.
//
bool contained( Exclusion &a, Exclusion &b )
{
    if( b.size() < a.size() ) return( false );

    Exclusion::iterator ib = b.begin();
    for( Exclusion::iterator ia = a.begin(); ia != a.end(); ++ia )
    {
        // move ib until pointers of ia and ib match
        while( ib != b.end() && ( ia->first )->GetSequence() != ( ib->first )->GetSequence() )
        {
            ++ib;
        }

        // end of b, return true only if also end of a
        if( ib == b.end() ) return ia == a.end();

        // pointers match, what about values
        if( ia->second != ib->second ) return false;
    }
    return true;
}

//
// compares two exclusion terms; returns:
// -1 if op1 <  op2
//  0 if op1 == op2
//  1 if op1 >  op2
//
int compareExclusionTerms( const ExclusionTerm& op1, const ExclusionTerm& op2 )
{
    assert( (( op1.first == op2.first ) && ( ( op1.first )->GetSequence() == ( op2.first )->GetSequence() ))
         || (( op1.first != op2.first ) && ( ( op1.first )->GetSequence() != ( op2.first )->GetSequence() )) );

    if( op1.first != op2.first )
        // return op1.first < op2.first ? -1 : 1;
        return ( op1.first )->GetSequence() < ( op2.first )->GetSequence() ? -1 : 1;
    else
        if( op1.second != op2.second )
            return op1.second < op2.second ? -1 : 1;
        else
            return 0;
}

//
// compares two exclusions (all their terms); returns:
// -1 if op1 <  op2
//  0 if op1 == op2
//  1 if op1 > op2
//
int compareExclusions(const Exclusion& op1, const Exclusion& op2)
{
    Exclusion::const_iterator i_op1 = op1.begin();
    Exclusion::const_iterator i_op2 = op2.begin();

    int comp = 0;
    while( i_op1 != op1.end() &&
           i_op2 != op2.end() )
    {
        comp = compareExclusionTerms( *i_op1, *i_op2 );
        if( comp != 0 ) break;
        ++i_op1;
        ++i_op2;
    }
    if( comp != 0 )
        return( comp );
    else
        if( op1.size() != op2.size() )
            return op1.size() < op2.size() ? -1 : 1;
        else
            return 0;
}

//
//
//
size_t Exclusion::ResultParamCount() const
{
    size_t count = 0;
    for( auto & term : col )
    {
        if( term.first->IsExpectedResultParam() )
        {
            ++count;
        }
    }
    return( count );
}

//
//
//
void Exclusion::Print() const
{
    for( auto & term : col )
        DOUT( ( term.first )->GetName() << L": " << term->second << L" " );
    DOUT( L"\n" );
}

//
// careful about changing this; ExclusionDeriver::contained depends on exclusion term sorting
//
bool ExclusionTermCompare::operator()(const ExclusionTerm& op1, const ExclusionTerm& op2) const
{
    assert(( (op1.first == op2.first) && ((op1.first)->GetSequence() == (op2.first)->GetSequence()) ) ||
           ( (op1.first != op2.first) && ((op1.first)->GetSequence() != (op2.first)->GetSequence()) ));
     
    return( -1 == compareExclusionTerms( op1, op2 ));
}

//
//
//
bool ExclusionSizeLess::operator() ( const Exclusion &excl1, const Exclusion &excl2 ) const
{
    return ( excl1.size() != excl2.size() ? excl1.size() < excl2.size() : ( -1 == compareExclusions( excl1, excl2 ) ) );
}

//
//
//
bool ExclIterCollectionPred::operator() ( const ExclusionCollection::iterator excl1, const ExclusionCollection::iterator excl2 ) const
{
    return( -1 == compareExclusions( *excl1, *excl2 ) );
}

}