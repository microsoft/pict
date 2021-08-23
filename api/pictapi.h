#if !defined( PICT_H )
#define PICT_H

#if !defined( IN )
#define IN
#endif

#if !defined( OUT )
#define OUT
#endif

#if !defined( OPT )
#define OPT
#endif

#if defined( __GNUC__ )
#define API_SPEC
#else
#define API_SPEC __stdcall
#endif

#if defined( __cplusplus )
extern "C" {
#endif

//
// Types
//

typedef void *        PICT_HANDLE;
typedef size_t        PICT_VALUE;
typedef PICT_VALUE *  PICT_RESULT_ROW;
typedef unsigned int  PICT_RET_CODE;

typedef struct _PICT_EXCLUSION_ITEM
{
    PICT_HANDLE Parameter;
    PICT_VALUE  ValueIndex;
} PICT_EXCLUSION_ITEM;

typedef struct _PICT_SEED_ITEM
{
    PICT_HANDLE Parameter;
    PICT_VALUE  ValueIndex;
} PICT_SEED_ITEM;


//
// Return codes
//

#define PICT_SUCCESS          0x00000000
#define PICT_OUT_OF_MEMORY    0xc0000001
#define PICT_GENERATION_ERROR 0xc0000002

//
// Defaults
//

#define PICT_PAIRWISE_GENERATION  2
#define PICT_DEFAULT_RANDOM_SEED  0

//
// Interface
//

// ////////////////////////////////////////////////////////////////////////////
//
// Allocates a new task. Everything happens in the context of a task.
// Tasks need to be deleted with DeleteTask.
// 
// Parameters:
//   None
//
// Returns:
//   Non-nullptr   Allocation succeeded (a handle is returned)
//   nullptr       Allocation failed
//
// ////////////////////////////////////////////////////////////////////////////

PICT_HANDLE
API_SPEC
PictCreateTask();

// ////////////////////////////////////////////////////////////////////////////
//
// Associates a model tree with a task.
//
// Parameters:
//   task      Valid handle of a task
//   model     Valid handle of the root of the model hierarchy
// 
// Returns:
//   Nothing
//
// ////////////////////////////////////////////////////////////////////////////

void
API_SPEC
PictSetRootModel
    (
    IN const PICT_HANDLE task,
    IN const PICT_HANDLE model
    );

// ////////////////////////////////////////////////////////////////////////////
//
// Adds an exclusion to the task.  Exclusions define combinations  which should
// not appear in the output. Such a combination has a variable number of items.
// Each item is a pair consisting of a pointer to the involved parameter and a
// zero-based index to its value. See the definition of PICT_EXCLUSION_ITEM.
//
// Parameters:
//   task                Valid handle of a task
//   exclusionItems      Array of PICT_EXCLUSION_ITEM's
//   exclusionItemCount  How many elements are passed in the array
// 
// Returns:
//   PICT_SUCCESS
//   PICT_OUT_OF_MEMORY
//
// ////////////////////////////////////////////////////////////////////////////

PICT_RET_CODE
API_SPEC
PictAddExclusion
    (
    IN const PICT_HANDLE         task,
    IN const PICT_EXCLUSION_ITEM exclusionItems[],
    IN       size_t              exclusionItemCount
    );

// ////////////////////////////////////////////////////////////////////////////
//
// Adds a seed to the task.Seeds define combinations that have to appear in the
// output (if they do not violate any exclusions).  A seed is a variable-length 
// list of parameter-value pairs. Each pair consists of a pointer  to the param
// and a zero-based index to its value. See the definition of PICT_SEED_ITEM.
//
// Parameters:
//   task           Valid handle of a task
//   seedItems      Array of PICT_SEED_ITEM's
//   seedItemCount  How many elements are passed in the array
// 
// Returns:
//   PICT_SUCCESS
//   PICT_OUT_OF_MEMORY
//
// ////////////////////////////////////////////////////////////////////////////

PICT_RET_CODE
API_SPEC
PictAddSeed
    (
    IN const PICT_HANDLE     task,
    IN const PICT_SEED_ITEM  seedItems[],
    IN       size_t          seedItemCount
    );

// ////////////////////////////////////////////////////////////////////////////
//
// Generates results.  Should only be called after all structures are fully set 
// up and models wired to the task. Otherwise expect access violations.
//
// Parameters:
//   task       Valid handle to a task
// 
// Returns:
//   PICT_SUCCESS           Success
//   PICT_OUT_OF_MEMORY     Not enough memory to complete generation
//   PICT_GENERATION_ERROR  An internal problem with the engine
//
// ////////////////////////////////////////////////////////////////////////////

PICT_RET_CODE
API_SPEC
PictGenerate
    (
    IN const PICT_HANDLE task
    );

// ////////////////////////////////////////////////////////////////////////////
//
// A helper function. Allocates and returns a pointer to a buffer big enough to
// hold one result row.  FreeResultBuffer should be used  when the buffer is no
// longer needed.
//
// Parameters:
//   task      Valid handle to a task
// 
// Returns:
//   Non-nullptr  Allocation succeeded (a handle is returned)
//   nullptr      Allocation failed
//
// ////////////////////////////////////////////////////////////////////////////

PICT_RESULT_ROW
API_SPEC
PictAllocateResultBuffer
    (
    IN const PICT_HANDLE task
    );

// ////////////////////////////////////////////////////////////////////////////
//
// Frees a buffer allocated by AllocateResultBuffer.
//
// Parameters:
//   resultRow  Buffer returned by AllocateResultBuffer
// 
// Returns:
//   Nothing
//
// ////////////////////////////////////////////////////////////////////////////

void
API_SPEC
PictFreeResultBuffer
    (
    IN const PICT_RESULT_ROW resultRow
    );

// ////////////////////////////////////////////////////////////////////////////
//
// Resets result retrieving. Lets us start from the begining of the result set.
//
// Parameters:
//   task       Valid handle to a task
// 
// Returns:
//   Nothing
//
// ////////////////////////////////////////////////////////////////////////////

void
API_SPEC
PictResetResultFetching
    (
    IN const PICT_HANDLE task
    );

// ////////////////////////////////////////////////////////////////////////////
//
// Fetches one result row,  fills a provided buffer with its values, then moves
// the "current row" pointer to the next row.  To get all the result rows, call 
// this function repeatedly until the return value == 0.
//
// Parameters:
//   task       Valid handle to a task
//   resultRow  Pointer to a chunk of memory  big enough to hold one result row
//              i. e. sizeof(PICT_VALUE) * [total number of params in the task]
//              Instead of allocating memory yourself use AllocateResultBuffer.
//
// Returns:
//              The number of rows still to be retrieved **before** the pointer
//              advances.  In other words,  this function returns the number of
//              rows remaining to be returned when we enter the function.  When 
//              the return value gets to zero, there is nothing else to return.
//              Any subsequent call will not affect the buffer and it will keep 
//              returning zero until a call to ResetResultFetching restarts the
//              result retrieving.
//
// ////////////////////////////////////////////////////////////////////////////

size_t
API_SPEC
PictGetNextResultRow
    (
    IN  const PICT_HANDLE     task,
    OUT       PICT_RESULT_ROW resultRow
    );

// ////////////////////////////////////////////////////////////////////////////
//
// Deletes a task
//
// Parameters:
//   task      Valid handle to a task allocated by CreateTask
// 
// Returns:
//   Nothing
//
// ////////////////////////////////////////////////////////////////////////////

void
API_SPEC
PictDeleteTask
    (
    IN const PICT_HANDLE task
    );

// ////////////////////////////////////////////////////////////////////////////
//
// Allocates a new model. Models need to be deleted with DeleteModel
// 
// Parameters:
//   randomSeed  A seed used to randomize the engine
//   
// Returns:
//   Non-nullptr    Allocation succeeded (a handle is returned)
//   nullptr        Allocation failed
//
// ////////////////////////////////////////////////////////////////////////////

PICT_HANDLE
API_SPEC
PictCreateModel
    (
    IN OPT unsigned int randomSeed = PICT_DEFAULT_RANDOM_SEED
    );

// ////////////////////////////////////////////////////////////////////////////
//
// Adds a parameter to a model
//
// Parameters:
//   model          Which model to add this parameter to
//   valueCount     Numer of values this parameter has
//   order          Order of combinations to be generated with this parameter
//   valueWeights   Array of weights, one per value, must be of ValueCount size
// 
// Returns:
//   Non-nullptr       Allocation succeeded (a handle is returned)
//   nullptr           Allocation failed
//
// ////////////////////////////////////////////////////////////////////////////

PICT_HANDLE
API_SPEC
PictAddParameter
    (
    IN     const PICT_HANDLE model,
    IN     size_t            valueCount,
    IN OPT unsigned int      order          = PICT_PAIRWISE_GENERATION,
    IN OPT unsigned int      valueWeights[] = nullptr
    );

//
//
//

size_t
API_SPEC
PictGetTotalParameterCount
    (
    IN const PICT_HANDLE task
    );

// ////////////////////////////////////////////////////////////////////////////
//
// Wires a model to another model to create a model-submodel hierarchy
//
// Parameters:
//   modelParent
//   modelChild
//   order       Order of combinations to be generated with this submodel
// 
// Returns:
//   PICT_SUCCESS
//   PICT_OUT_OF_MEMORY
//
// ////////////////////////////////////////////////////////////////////////////

PICT_RET_CODE
API_SPEC
PictAttachChildModel
    (
    IN     const PICT_HANDLE modelParent,
    IN     const PICT_HANDLE modelChild,
    IN OPT unsigned int      order = PICT_PAIRWISE_GENERATION
    );

// ////////////////////////////////////////////////////////////////////////////
//
// Deallocates a model, all its parameters *AND* all submodels attached to it
//
// Parameters:
//   model  Valid handle to a model allocated by CreateModel
// 
// Returns:
//   Nothing
//
// ////////////////////////////////////////////////////////////////////////////

void
API_SPEC
PictDeleteModel
    (
    IN const PICT_HANDLE model
    );

#if defined( __cplusplus )
} // extern "C"
#endif

#endif // PICT_H
