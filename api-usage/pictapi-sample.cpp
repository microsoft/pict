#include <iostream>
#include <cassert>
using namespace std;

#include "pictapi.h"

#define PAIRWISE 2

#define checkNull( x )                                    \
    if( nullptr == x )                                    \
    {                                                     \
        wcout << L"Error: Out of memory" << endl;         \
        goto cleanup;                                     \
    }

#define checkRetCode( x )                                 \
    switch( x )                                           \
    {                                                     \
    case PICT_SUCCESS:                                    \
        break;                                            \
    case PICT_OUT_OF_MEMORY:                              \
        wcout << L"Error: Out of memory" << endl;         \
        goto cleanup;                                     \
    case PICT_GENERATION_ERROR:                           \
        wcout << L"Error: Internal engine error" << endl; \
        goto cleanup;                                     \
    default:                                              \
        assert( ! L"Unexpected error code" );             \
        goto cleanup;                                     \
    }

void __cdecl wmain()
{
    PICT_RET_CODE ret = PICT_SUCCESS;

    PICT_HANDLE task = PictCreateTask();

    //
    // In a general case,  models might form a tree-like hierarchy
    // Let's try with only one model for now,  pairwise is default
    //

    PICT_HANDLE model = PictCreateModel();
    checkNull( task );
    checkNull( model );

    //
    // The root of the model tree should be attached  to  the task
    // In this simple case,  we will attach the only model we have
    //
    
    PictSetRootModel( task, model );

    //
    // We will add five parameters with different number of values
    // to the model.  Let's store pointers to our parameters since
    // we will use them to define exclusions.
    //
    // The order of combinations for each parameter must be greater 
    // than 0  and less or equal the number  of parameters defined 
    // in the model, we will use order = 2, i.e. pairwise.
    //
    // An array of weights determines which values are more likely
    // to be picked.

    //
    // The order of calls here  determines  the order of values in 
    // the resulting test cases.
    //

    unsigned int weights[] = {1, 2, 1, 1};
    PICT_HANDLE p1 = PictAddParameter( model, 4, PAIRWISE, weights );
    checkNull( p1 );

    PICT_HANDLE p2 = PictAddParameter( model, 3, PAIRWISE );
    checkNull( p2 );

    PICT_HANDLE p3 = PictAddParameter( model, 5, PAIRWISE );
    checkNull( p3 );

    PICT_HANDLE p4 = PictAddParameter( model, 2, PAIRWISE );
    checkNull( p4 );

    PICT_HANDLE p5 = PictAddParameter( model, 4, PAIRWISE );
    checkNull( p5 );

    //
    // Exclusions determine  which combinations should not show up
    // in the output. An exclusion is a list of param-value pairs.
    // Make sure you're not exceeding a value count for each param
    // when using 0-based value indices.
    //

    //
    // Exclude a combination of
    //   (1st value of 1st param) and
    //   (1st value of 2nd param)
    //

    const size_t EXCLUSION_1_SIZE = 2;

    PICT_EXCLUSION_ITEM excl1[ EXCLUSION_1_SIZE ];
    
    excl1[ 0 ].Parameter  = p1;
    excl1[ 0 ].ValueIndex = 0;
    
    excl1[ 1 ].Parameter  = p2;
    excl1[ 1 ].ValueIndex = 0;

    ret = PictAddExclusion( task, excl1, EXCLUSION_1_SIZE );
    checkRetCode( ret );

    //
    // Exclude a combination of
    //   (2nd value of 4th param) and
    //   (3rd value of 5th param)
    //

    const size_t EXCLUSION_2_SIZE = 2;

    PICT_EXCLUSION_ITEM excl2[ EXCLUSION_2_SIZE ];

    excl2[ 0 ].Parameter  = p4;
    excl2[ 0 ].ValueIndex = 1;
    
    excl2[ 1 ].Parameter  = p5;
    excl2[ 1 ].ValueIndex = 2;

    ret = PictAddExclusion( task, excl2, EXCLUSION_2_SIZE );
    checkRetCode( ret );

    //
    // Seeding rows tell the engine which combinations must appear
    // in the output. If they don't violate exclusions, of course.
    // Let's add one of those; it is similar to adding exclusions.
    //

    //
    // Make sure that a test case  where all parameters are set to
    // their 2nd values is present in the generated output.  Again
    // the 0-based indices cannot go beyond the number of possible
    // values for any of the parameters.
    //

    const size_t SEED_1_SIZE = 5;

    PICT_SEED_ITEM seed1[ SEED_1_SIZE ];
    
    seed1[ 0 ].Parameter  = p1;
    seed1[ 0 ].ValueIndex = 1;
    
    seed1[ 1 ].Parameter  = p2;
    seed1[ 1 ].ValueIndex = 1;
    
    seed1[ 2 ].Parameter  = p3;
    seed1[ 2 ].ValueIndex = 1;
    
    seed1[ 3 ].Parameter  = p4;
    seed1[ 3 ].ValueIndex = 1;
    
    seed1[ 4 ].Parameter  = p5;
    seed1[ 4 ].ValueIndex = 1;

    ret = PictAddSeed( task, seed1, SEED_1_SIZE );
    checkRetCode( ret );

    //
    // The main event: generation.
    //

    ret = PictGenerate( task );
    checkRetCode( ret );

    //
    // The result is a collection of rows.Each row is a collection
    // of values:  one value for every parameter in the order they
    // were added to the models; i.e. in the order of AddParameter
    // calls. We can only get one row at a time.
    //
    // Per exclusion 1, we should not see "0 0" as first two items
    // in any row. Per exclusion 2,  "1 2" will not appear as last
    // two items in any of the rows.  Also, per seed 1 we must see
    // a test "1 1 1 1 1".
    //

    //
    // First let's allocate a piece of memory big enough to hold a
    // row. Every row of the result is of the same size because it
    // contains the same number of values. Each value corresponds
    // to one parameter we defined earlier.
    //
    // We could allocate the memory by hand since we know how many
    // parameters we have in all models of the task  (moreover, we 
    // have a handy PictGetTotalParameterCount function available)
    // but PictAllocateResultBuffer can do it for us.
    //

    PICT_RESULT_ROW row = PictAllocateResultBuffer( task );
    checkNull( row );

    //
    // We will get rows by repeatedly calling PictGetNextResultRow
    // until it returns false. Before we do that however a call to
    // PictResetResultFetching will reset the retrieval procedure.
    //
    // If  the result fetching  ever needs to be repeated for this
    // task,  a call to PictResetResultFetching  will let us start 
    // over.
    //

    size_t paramCount = PictGetTotalParameterCount( task );

    PictResetResultFetching( task );

    while( PictGetNextResultRow( task, row ))
    {
        for( size_t index = 0; index < paramCount; ++index )
        {
            wcout << static_cast<unsigned int>( row[ index ] )<< L" ";
        }
        wcout << endl;
    }

    //
    // Memory allocated for the buffer needs to be freed.
    //

    PictFreeResultBuffer( row );

cleanup:

    if( model != nullptr )
    {
        PictDeleteModel( model );
    }

    if( task != nullptr )
    {
        PictDeleteTask( task );
    }
};
