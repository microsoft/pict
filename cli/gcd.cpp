#include "gcd.h"
using namespace std;

#define FAILED(err) (ErrorCode::ErrorCode_Success != (err))

namespace pictcli_gcd
{

//
//
//
ErrorCode GcdRunner::generateResults( IN CModelData& modelData, IN bool justNegative )
{
    // translate all data to gcd-readable format, build exclusions etc.
    CGcdData gcdData( modelData );
    ErrorCode err = gcdData.TranslateToGCD();
    if( FAILED( err )) return( err );

    // check if we excluded all values of any parameter; generation will return nothing if that's the case
    if( gcdData.CheckEntireParameterExcluded() )
    {
        return( ErrorCode::ErrorCode_BadConstraints );
    }

    // save all one-element exclusions, they'll be used for warnings later
    _result.SolverWarnings       = gcdData.GetConstraintWarnings();
    _result.SingleItemExclusions = gcdData.GetSingleItemExclusions();

    if( modelData.Verbose )
    {
        modelData.PrintModelContents( L"*** AFTER MODEL IS PARSED ***" );
    }

    // run the generation from bottom up
    Model* model = gcdData.GetRootModel();
    model->SetRandomSeed( modelData.RandSeed );
    try
    {
        for( auto & submodel : model->GetSubmodels() )
        {
            // each submodel may assign different order to parameters
            if( !gcdData.FixParamOrder( submodel ))
            {
                return( ErrorCode::ErrorCode_BadModel );
            }

            if( modelData.Verbose )
            {
                modelData.PrintModelContents( L"*** AFTER ORDER IS FIXED ***" );
            }

            submodel->Generate();
        
            modelData.AddToTotalCombinationsCount( submodel->GetTotalCombinationsCount() );
            modelData.AddToRemainingCombinationsCount( submodel->GetRemainingCombinationsCount() );
        }

        if( !gcdData.FixParamOrder( model ))
        {
            return( ErrorCode::ErrorCode_BadModel );
        }

        if( modelData.Verbose )
        {
            modelData.PrintModelContents( L"*** AFTER ORDER IS FIXED ***" );
        }

        model->Generate();

        modelData.AddToTotalCombinationsCount( model->GetTotalCombinationsCount() );
        modelData.AddToRemainingCombinationsCount( model->GetRemainingCombinationsCount() );

        if( modelData.Verbose )
        {
            modelData.PrintModelContents( L"*** AFTER GENERATION ***" );
        }
    }
    catch( GenerationError e )
    {
        switch( e.GetErrorType() )
        {
        case ErrorType::OutOfMemory:
            PrintMessage( GcdError, L"Out of memory. Use smaller /o or simplify your model." );
            break;
        case ErrorType::GenerationCancelled: // not used in PICT.EXE
        case ErrorType::TooManyRows:         // not used in PICT.EXE
        case ErrorType::Unknown:
        case ErrorType::GenerationFailure:
            wstring msg = L"Internal error\n";
            msg += L"As a workaround run the tool with parameter /r a few times and see if any of the iterations produces a result.\n";
            msg += L"If the result is produced, it is guaranteed to be valid and it is safe to use.";
            PrintMessage( GcdError, msg.data() );
            break;
        }
#ifdef _DEBUG
        assert( false );
        throw;
#endif
        return( ErrorCode::ErrorCode_GenerationError );
    }

    translateResults( modelData, model->GetResults(), justNegative );

    return( ErrorCode::ErrorCode_Success );
}

//
//
//
void GcdRunner::translateResults( IN CModelData&       modelData,
                                  IN ResultCollection& resultCollection,
                                  IN bool              justNegative )
{
    for( auto & row : resultCollection )
    {
        bool isNegativeTestCase = false;
        
        wstrings rowText, decoratedRowText;
        for( size_t pindex = 0; pindex < row.size(); ++pindex )
        {
            auto vindex = row[ pindex ];
            wstring name, decoratedName;
            if( Parameter::UndefinedValue == vindex )
            {
                name = decoratedName = L"?";
            }
            else
            {
                CModelValue& value = modelData.Parameters[ pindex ].Values[ vindex ];

                if( !value.IsPositive() )
                {
                    decoratedName = charToStr( modelData.InvalidPrefix );
                    isNegativeTestCase = true;
                }
                name = value.GetNextName();
                decoratedName += name;
            }
            rowText.push_back( name );
            decoratedRowText.push_back( decoratedName );
        }

        if( !justNegative || ( justNegative && isNegativeTestCase ))
        {
            CRow testCase( rowText, decoratedRowText, isNegativeTestCase );
            _result.TestCases.push_back( testCase );
        }
    }
}

//
//
//
ErrorCode GcdRunner::Generate()
{
    // we have to run production twice: once for positive and once negative values

    // make a copy
    CModelData modelData2 = _modelData;

    // the "positive" run; remove all negative values 
    if( _modelData.HasNegativeValues() )
    {
        _modelData.RemoveNegativeValues();
    }

    ErrorCode err = generateResults( _modelData, false );
    if( FAILED( err )) return( err );

    // the "negative" run; only if any negative values exist
    if( modelData2.HasNegativeValues() )
    {
        // if negative values exist then positive run may have already warned about 
        // non-existing problems with the constraints, discard the set of warnings
        // as a new one will be produced and we don't want any dupliactes
        _result.SolverWarnings.clear();
        _result.SingleItemExclusions.clear();

        err = generateResults( modelData2, true );
        if( FAILED( err )) return( err );
    }

    return( ErrorCode::ErrorCode_Success );
}

}
