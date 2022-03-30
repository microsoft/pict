#include <algorithm>
using namespace std;

#include "pictapi.h"
#include "generator.h"
using namespace pictcore;

//
// forward declarations
//
void generate( Model* model );

#define NO_CONST_HANDLE(X) const_cast< void * >( X )

//
//
//
PICT_HANDLE
API_SPEC
PictCreateTask()
{
    Task* taskObj = new Task();
    return( static_cast<PICT_HANDLE>( taskObj ) );
}

//
//
//
void
API_SPEC
PictSetRootModel
    (
    IN const PICT_HANDLE task,
    IN const PICT_HANDLE model
    )
{
    Task* taskObj = static_cast<Task*>( NO_CONST_HANDLE( task ));
    taskObj->SetRootModel( static_cast<Model*>( NO_CONST_HANDLE( model )));
}

//
//
//
PICT_RET_CODE
API_SPEC
PictAddExclusion
    (
    IN const PICT_HANDLE         task,
    IN const PICT_EXCLUSION_ITEM exclusionItems[],
    IN       size_t              exclusionItemCount
    )
{
    Task* taskObj = static_cast<Task*>( NO_CONST_HANDLE( task ));
    
    try
    {
        Exclusion exclusion;
        for( size_t index = 0; index < exclusionItemCount; ++index )
        {
            exclusion.insert( make_pair( 
                static_cast<Parameter*> ( NO_CONST_HANDLE( exclusionItems[ index ].Parameter  )),
                static_cast<int>        (                  exclusionItems[ index ].ValueIndex ))
                );
        }
        bool ret = taskObj->AddExclusion( exclusion );
        return( ret ? PICT_SUCCESS : PICT_OUT_OF_MEMORY );
    }
    catch( ... )
    {
        return( PICT_OUT_OF_MEMORY );
    }
}

//
//
//
PICT_RET_CODE
API_SPEC
PictAddSeed
    (
    IN const PICT_HANDLE     task,
    IN const PICT_SEED_ITEM  seedItems[],
    IN       size_t          seedItemCount
    )
{
    Task* taskObj = static_cast<Task*>( NO_CONST_HANDLE( task ));
    
    try
    {
        RowSeed seed;
        for( size_t index = 0; index < seedItemCount; ++index )
        {
            seed.insert( make_pair( 
                static_cast<Parameter*> ( NO_CONST_HANDLE( seedItems[ index ].Parameter  )),
                static_cast<int>        (                  seedItems[ index ].ValueIndex ))
                );
        }
        taskObj->AddRowSeed( seed );
        return( PICT_SUCCESS );
    }
    catch( ... )
    {
        return( PICT_OUT_OF_MEMORY );
    }
}

//
//
//
PICT_RET_CODE
API_SPEC
PictGenerate
    (
    IN const PICT_HANDLE task
    )
{
    Task* taskObj = static_cast<Task*>( NO_CONST_HANDLE( task ));
    try
    {
        taskObj->PrepareForGeneration();
    }
    catch( ... )
    {
        return( PICT_GENERATION_ERROR );
    }
    
    Model* root = taskObj->GetRootModel();
    try
    {
        generate( root );
    }
    catch( GenerationError e )
    {
        switch( e.GetErrorType() )
        {
        case ErrorType::OutOfMemory:
            return( PICT_OUT_OF_MEMORY );
        case ErrorType::GenerationCancelled:
        case ErrorType::TooManyRows:
        case ErrorType::Unknown:
        case ErrorType::GenerationFailure:
        default:
            return( PICT_GENERATION_ERROR );
        }
    }
    return( PICT_SUCCESS );
}

//
//
//
PICT_RESULT_ROW
API_SPEC
PictAllocateResultBuffer
    (
    IN const PICT_HANDLE task
    )
{
    Task* taskObj = static_cast<Task*>( NO_CONST_HANDLE( task ));
    size_t totalParamCount = taskObj->GetTotalParameterCount();
    
    PICT_RESULT_ROW row = static_cast<PICT_RESULT_ROW>( malloc( totalParamCount * sizeof( PICT_VALUE ) ) );

    return( row );
}

//
//
//
void
API_SPEC
PictFreeResultBuffer
    (
    IN const PICT_RESULT_ROW resultBuffer
    )
{
    free( resultBuffer );
}

//
//
//
void
API_SPEC
PictResetResultFetching
    (
    IN const PICT_HANDLE task
    )
{
    Task* taskObj = static_cast<Task*>( NO_CONST_HANDLE( task ));
    taskObj->ResetResultFetching();
}

//
//
//
size_t
API_SPEC
PictGetNextResultRow
    (
    IN  const PICT_HANDLE     task,
    OUT       PICT_RESULT_ROW resultRow
    )
{
    Task* taskObj = static_cast<Task*>( NO_CONST_HANDLE( task ));
    
    ResultCollection::iterator i_row = taskObj->GetNextResultRow();

    if( i_row != taskObj->GetResults().end() )
    {
        for( size_t index = 0; index < i_row->size(); ++index )
        {
            resultRow[ index ] = (*i_row)[ index ];
        }
    }
    
    return( distance( i_row, taskObj->GetResults().end() ));
}

//
//
//
void
API_SPEC
PictDeleteTask
    (
    IN const PICT_HANDLE task
    )
{
    Task* taskObj = static_cast<Task*>( NO_CONST_HANDLE( task ));
    delete( taskObj );
}

//
//
//
PICT_HANDLE
API_SPEC
PictCreateModel
    (
    IN OPT unsigned int randomSeed
    )
{
    Model* modelObj = new Model( L"",
                                 GenerationType::MixedOrder,
                                 0,
                                 randomSeed );
    
    return( static_cast<PICT_HANDLE>( modelObj ));   
}

//
//
//
PICT_HANDLE
API_SPEC
PictAddParameter
    (
    IN     const PICT_HANDLE model,
    IN           size_t      valueCount,
    IN OPT unsigned int      order,
    IN OPT unsigned int      valueWeights[]
    )
{
    Model* modelObj = static_cast<Model*>( NO_CONST_HANDLE( model ));

    Parameter* param = new Parameter( order,
                                      static_cast<int>( modelObj->GetParameters().size() + 1 ),
                                      static_cast<int>( valueCount ),
                                      L"",
                                      false );
    try
    {
        if( nullptr != param )
        {
            if( nullptr != valueWeights )
            {
                std::vector<int> weights;
                weights.reserve( valueCount );
                weights.insert( weights.begin(), valueWeights, valueWeights + valueCount );
                param->SetWeights( move( weights ));
            }

            modelObj->AddParameter( param );
        }
    }
    catch( ... )
    {
        param = nullptr;
    }

    return( param );
}

//
//
//
size_t
API_SPEC
PictGetTotalParameterCount
    (
    IN const PICT_HANDLE task
    )
{
    Task* taskObj = static_cast<Task*>( NO_CONST_HANDLE( task ));
    return( taskObj->GetTotalParameterCount() );
}

//
//
//
PICT_RET_CODE
API_SPEC
PictAttachChildModel
    (
    IN const PICT_HANDLE modelParent,
    IN const PICT_HANDLE modelChild,
    IN OPT unsigned int  order
    )
{
    Model* parent = static_cast<Model*>( NO_CONST_HANDLE( modelParent ));
    Model* child  = static_cast<Model*>( NO_CONST_HANDLE( modelChild  ));
    try
    {
        child->SetOrder( order );
        if ( parent->GetOrder() < static_cast<int>( order ))
        {
            parent->SetOrder( order );
        }
        
        parent->AddSubmodel( child );
        return( PICT_SUCCESS );
    }
    catch( ... )
    {
        return( PICT_OUT_OF_MEMORY );
    }
}

//
//
//
void
API_SPEC
PictDeleteModel
    (
    IN const PICT_HANDLE model
    )
{
    Model* modelObj = static_cast<Model*>( NO_CONST_HANDLE( model ));

    // delete all params first
    // since GetParameters() will return all params for this and all submodels, do it here
    for( Parameter* param : modelObj->GetParameters() )
    {
        delete( param );
    }

    // and this model, Model::~Model will clean up all submodels too
    delete( modelObj );
}

//
//
//
void generate( Model* model )
{
    for ( Model* subModel : model->GetSubmodels() )
    {
        generate(subModel);
    }
    model->Generate();
}
