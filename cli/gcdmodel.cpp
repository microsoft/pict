#include "cmdline.h"
#include "gcdexcl.h"
#include "gcdmodel.h"
using namespace std;

namespace pictcli_gcd
{

//
// when constraints are too strict and entire parameters get excluded from
// generation, the process can't continue
//
bool CGcdData::CheckEntireParameterExcluded()
{
    // key is a parameter, parameter has a set of values
    map< Parameter*, set< int > > paramMap;
    set< int > emptySet;

    // walk through all exclusions, pick one-element ones, and add them to the map
    for( auto & exclusion : Exclusions )
    {
        if( 1 == exclusion.size() )
        {
            ExclusionTerm& term = const_cast<ExclusionTerm&> ( *( exclusion.begin() ) );
            auto result = paramMap.insert( make_pair( term.first, emptySet ) );

            set< int >& values = ( result.first )->second;
            values.insert( term.second );
        }
    }

    // if any of the params in the map contains all elements then the entire parameter is excluded
    for( auto & parameter : paramMap )
    {
        if( ( parameter.first )->GetValueCount() == static_cast<int> ( parameter.second.size() ))
        {
            auto found = _modelData.FindParameterByGcdPointer( parameter.first );
            assert( found != _modelData.Parameters.end() );

            wstring param = L"'" + found->Name + L"'";
            PrintMessage( InputDataError, L"Too restrictive constraints. All values of parameter",
                          param.c_str(), L"got excluded." );
            return( true );
        }
    }

    return( false );
}

//
//
//
wstrings CGcdData::GetSingleItemExclusions()
{
    wstrings collection;

    for( auto & exclusion : Exclusions )
    {
        if( 1 == exclusion.size() )
        {
            ExclusionTerm& term = const_cast<ExclusionTerm&> ( * exclusion.begin() );
            auto found = _modelData.FindParameterByGcdPointer( term.first );
            assert( found != _modelData.Parameters.end() );

            wstring text = found->Name;
            text += L": ";
            text += found->Values.at( term.second ).GetPrimaryName();
            collection.push_back( text );
        }
    }

    return collection;
}

//
//
//
bool CGcdData::FixParamOrder( IN Model* submodel )
{
    // clean order assignments for non-result params, set order of result params to 1
    for( auto & param : _modelData.Parameters )
    {
        if( param.IsResultParameter )
        {
            param.GcdPointer->SetOrder( 1 );
        }
        else
        {
            param.GcdPointer->SetOrder( UNDEFINED_ORDER );
        }
    }

    // If this is an actual submodel (a model other than the root), by now it will have its
    // order defined so use it across all its parameters
    if( submodel != _task.GetRootModel() )
    {
        for( auto & param : submodel->GetParameters() )
        {
            if( UNDEFINED_ORDER == param->GetOrder() )
            {
                param->SetOrder( submodel->GetOrder() );
            }
        }
    }

    // For the root model, use orders specified in parameter definitions or if none was
    // defined, use the default order of the model
    else
    {
        // order from param definitions
        for( auto & param : submodel->GetParameters() )
        {
            if( UNDEFINED_ORDER == param->GetOrder() )
            {
                auto p = _modelData.FindParameterByGcdPointer( param );
                assert( p != _modelData.Parameters.end() );
                if( p->Order != static_cast<unsigned int>(UNDEFINED_ORDER) )
                {
                    // TODO: add verification of Order
                    // if p->Order > model->parameters.count - model.ResultParameters.count then error out
                    param->SetOrder( p->Order );
                }
                else
                {
                    param->SetOrder( submodel->GetOrder() );
                }
            }
        }
    }

    return( true );
}

//
//
//
typedef map< CModelParameter*, Parameter* > CParamMap;

//
// the main proc translating the model gathered from the UI to one used by the engine
//
ErrorCode CGcdData::TranslateToGCD()
{
    Model* rootModel = new Model( L"", GenerationType::MixedOrder, _modelData.Order, _modelData.RandSeed );
    Models.push_back( rootModel );

    _task.SetRootModel( rootModel );
    _task.SetGenerationMode( _modelData.GenerationMode );
    if( _modelData.GenerationMode == GenerationMode::Approximate )
    {
        _task.SetMaxRandomTries( _modelData.MaxApproxTries );
    }

    // first resolve all submodels:
    //  each submodel will be a new model linked to a root model
    //  all parameters not assigned to any submodel will be linked to the root

    // create a map of all parameter iterators, this will guide the rest of the translation
    CParamMap paramMap;

    for( size_t index = 0; index < _modelData.Parameters.size(); ++index )
    {
        CModelParameter& param = _modelData.Parameters[ index ];

        Parameter* gcdParam = new Parameter( UNDEFINED_ORDER, static_cast<int>(index),
                                             static_cast<int>( param.Values.size() ),
                                             param.Name, param.IsResultParameter );

        // find out and assign weights to values
        // we don't have to care for the clean-up of the structure, Parameter's destructor will clean it
        vector< int > weightVector;
        for( auto & value : param.Values )
        {
            weightVector.push_back( value.GetWeight() );
        }
        gcdParam->SetWeights( weightVector );

        // store the structure, update its back pointer and 
        Parameters.push_back( gcdParam );
        param.GcdPointer = gcdParam;
        
        // store the pointer in a safe place for later
        paramMap.insert( make_pair( &param, gcdParam ) );
    }

    // we will store all params assigned to submodels here
    set<Parameter*> usedInSubmodels;

    // now go through all the submodels and wire up parameters to models
    for( auto & submodel : _modelData.Submodels )
    {
        Model* gcdModel = new Model( L"", GenerationType::MixedOrder, submodel.Order, _modelData.RandSeed );
        Models.push_back( gcdModel );

        for( auto & idx_param : submodel.Parameters )
        {
            // idx_param is an index of a parameter in ModelData.Parameters collection
            // to find a submodel in a guiding map let's locate that parameter and get an iterator
            vector< CModelParameter >::iterator i_param = _modelData.Parameters.begin() + idx_param;
            CParamMap::iterator found = paramMap.find( &(*i_param) );
            assert( found != paramMap.end() );

            // insert
            gcdModel->AddParameter( found->second );
            usedInSubmodels.insert( found->second );
        }
    }

    // wire up all submodels to a root model
    for( auto & model : Models )
    {
        if( rootModel != model )
        {
            rootModel->AddSubmodel( model );
        }
    }

    // for outstanding parameters we have two options:
    // 1. if any submodels were explicitly defined by a user we should create a submodel for each
    //    outstanding parameter; all such submodels should be uplinked to the root
    // 2. if no submodels were defined we just put params directly to the root
    if( usedInSubmodels.size() != paramMap.size() )
    {
        if( _modelData.Submodels.size() > 0 )
        {
            for( auto & iparam : paramMap )
            {
                if( usedInSubmodels.find( iparam.second ) != usedInSubmodels.end() )
                {
                    continue;
                }

                Model* subModel = new Model( L"", GenerationType::MixedOrder, 1, _modelData.RandSeed );
                Models.push_back( subModel );
                rootModel->AddSubmodel( subModel );

                subModel->AddParameter( iparam.second );
            }
        }
        else
        {
            for( auto & iparam : paramMap )
            {
                rootModel->AddParameter( iparam.second );
            }
        }
    }

    // add seeding rows
    for( auto & seed : _modelData.RowSeeds )
    {
        RowSeed rowSeed;
        for( auto & item : seed )
        {
            // find a pointer to Parameter and ordinal number of the value
            vector<CModelParameter>::iterator param = _modelData.FindParameterByName( item.first );
            assert( param != _modelData.Parameters.end() );
            int nVal = param->GetValueOrdinal( item.second, _modelData.CaseSensitive );
            if( nVal >= 0 )
            {
                rowSeed.insert( make_pair( param->GcdPointer, nVal ) );
            }
        }
        _task.AddRowSeed( rowSeed );
    }

    // make sure all order fields in models are set appropriately
    if( !fixModelAndSubmodelOrder() )
    {
        return( ErrorCode::ErrorCode_BadModel );
    }
    
    // add exclusions for negative values
    addExclusionsForNegativeRun();

    // add user-specified exclusions now
    
    // parse the constraints and make exclusions out of them
    ConstraintsInterpreter interpreter( _modelData, Parameters );
    if( !interpreter.ConvertToExclusions( Exclusions ) )
    {
        return( ErrorCode::ErrorCode_BadConstraints );
    }
    _constraintWarnings.assign( interpreter.GetWarnings().begin(), interpreter.GetWarnings().end() );

    if( _modelData.Verbose )
    {
        PrintLogHeader( L"Initial set of exclusions" );
        PrintGcdExclusions();
    }

    // add each exclusion to that model in the hierarchy which is the most suitable:
    //  1. a subtree of that model must have all the parameters of the exclusion
    //  2. no lower subtree satisfies the condition 1)
    for( auto & excl : Exclusions )
    {
        _task.AddExclusion( const_cast<Exclusion&> ( excl ) );
    }

    _task.PrepareForGeneration();

    // at this point we don't need gcdData.Exclusions anymore
    Exclusions.clear();
    __insert( Exclusions, _task.GetExclusions().begin(), _task.GetExclusions().end() );

    if( _modelData.Verbose )
    {
        PrintLogHeader( L"After derivation" );
        PrintGcdExclusions();
    }

    return( ErrorCode::ErrorCode_Success );
}

//
//
//
void CGcdData::PrintGcdExclusions()
{
    for( auto & exclusion : Exclusions )
    {
        for( auto & term : exclusion )
        {
            size_t paramIdx;
            for( paramIdx = 0; paramIdx < Parameters.size(); ++paramIdx )
            {
                Parameter* param = Parameters[ paramIdx ];
                if( param == term.first ) break;
            }
            CModelParameter& pp = _modelData.Parameters[ paramIdx ];
            CModelValue&     vv = pp.Values[ term.second ];
            wcerr << L"( " << pp.Name << L": " << vv.GetPrimaryName() << L" ) ";
        }
        wcerr << endl;
    }
    wcerr << L"Count: " << (unsigned int) Exclusions.size() << endl;
}

//
//
//
void CResult::PrintOutput( CModelData& modelData, wostream& wout )
{
    wstring encodingPrefix;
    setEncodingType( modelData.GetEncoding(), encodingPrefix );
    wout << encodingPrefix;

    for( vector< CModelParameter >::iterator i_param =  modelData.Parameters.begin();
                                             i_param != modelData.Parameters.end();
                                             i_param++ )
    {
        if( i_param != modelData.Parameters.begin() ) wout << RESULT_DELIMITER;
        wout << i_param->Name;
    }
    wout << endl;

    for( vector< CRow >::iterator i_row =  TestCases.begin();
                                  i_row != TestCases.end();
                                  i_row++ )
    {
        for( wstrings::iterator i_value =  i_row->DecoratedValues.begin();
                                i_value != i_row->DecoratedValues.end();
                                i_value++ )
        {
            if( i_value != i_row->DecoratedValues.begin() )
            {
                wout << RESULT_DELIMITER;
            }
            wout << *i_value;
        }
        wout << endl;
    }
}

//
//
//
void CResult::PrintConstraintWarnings()
{
    if( SingleItemExclusions.size() > 0 )
    {
        wstring text = L"Restrictive constraints. Output will not contain following values: ";
        for( auto item : SingleItemExclusions )
        {
            text += L"\n  " + item;
        }
        PrintMessage( ConstraintsWarning, text.c_str() );
    }

    for( auto & warn : SolverWarnings )
    {
        PrintMessage( ConstraintsWarning, warn.c_str() );
    }
}

//
//
//
void CResult::PrintStatistics()
{
    PrintStatisticsCaption( wstring( L"Generated tests" ) );
    wcout << static_cast<int> ( TestCases.size() ) << endl;
}

//
// Figure out order for all model elements with UNDEFINED_ORDER or for
// root model with MAXIMUM_ORDER.
//
bool CGcdData::fixModelAndSubmodelOrder()
{
    if( _modelData.Order < 1 
     && _modelData.Order != UNDEFINED_ORDER )
    {
        PrintMessage( InputDataError, L"Order cannot be smaller than 1" );
        return( false );
    }

    Model* rootModel = _task.GetRootModel();

    // If the order given as an argument to the program has not been
    // explicitly defined it defaults to 2; if it's been provided but
    // given the "max" value, it defaults to MAXIMUM_ORDER. In both
    // cases, the order value must be recomputed and set to the real
    // maximum to avoid failing the later sanity checks.
    size_t inputParamCount = _modelData.TotalParameterCount() - _modelData.ResultParameterCount();

    if( _modelData.ProvidedArguments.find( SWITCH_ORDER ) == _modelData.ProvidedArguments.end()
     || _modelData.Order == MAXIMUM_ORDER )
    {
        // if submodels were defined, don't need any params, otherwise order = params without submodels
        if( _modelData.Submodels.size() > 0 )
        {
            if( _modelData.Order > static_cast<int>( rootModel->GetSubmodelCount() ) )
            {
                _modelData.Order = rootModel->GetSubmodelCount();
            }
        }
        else
        {
            if( inputParamCount > 0 && _modelData.Order > static_cast<int>( inputParamCount ) )
            {
                _modelData.Order = static_cast<int>( inputParamCount );
            }
        }

        rootModel->SetOrder( _modelData.Order );
    }

    // now perform standard check on the order
    if( _modelData.Submodels.size() > 0 )
    {
        // order of combinations provided to the tool cannot be bigger than number of submodels
        if( _modelData.Order > static_cast<int>( rootModel->GetSubmodelCount() ) )
        {
            PrintMessage( InputDataError, L"Order cannot be larger than total number of submodels and oustanding parameters" );
            return( false );
        }
    }
    else
    {
        // check that the order is at most the number of params
        if( _modelData.Order > (int) inputParamCount )
        {
            PrintMessage( InputDataError, L"Order cannot be larger than number of parameters" );
            return( false );
        }
    }

    // now that we have the model order calculated, fix all submodels in which order is still UNDEFINED
    for( auto & model : Models )
    {
        if( model != rootModel && UNDEFINED_ORDER == model->GetOrder() )
        {
            model->SetOrder( min( (int) model->GetParameters().size(), _modelData.Order ) );
        }
    }

    // perform checks on the submodels
    for( auto & model : Models )
    {
        if( model->GetOrder() < 1 )
        {
            PrintMessage( InputDataError, L"Order of a submodel should be at least 1" );
            return( false );
        }

        // only for models that do not contain other models
        if( 0 == model->GetSubmodelCount() )
        {
            if( model->GetOrder() > (int) ( model->GetParameters().size() ) )
            {
                PrintMessage( InputDataError, L"Order of a submodel cannot be larger than number of involved parameters" );
                return( false );
            }
        }
    }

    return( true );
}

//
// negative runs are accomplished by adding synthetic exclusions such that
// no two negative values can co-exist in one test case
//
void CGcdData::addExclusionsForNegativeRun()
{
    for( size_t param1Idx = 0; param1Idx < _modelData.Parameters.size(); ++param1Idx )
    {
        CModelParameter& param1 = _modelData.Parameters[ param1Idx ];

        for( size_t val1Idx = 0; val1Idx < param1.Values.size(); ++val1Idx )
        {
            CModelValue& val1 = param1.Values[ val1Idx ];

            if( !val1.IsPositive() )
            {
                for( size_t param2Idx = param1Idx + 1; param2Idx < _modelData.Parameters.size(); ++param2Idx )
                {
                    CModelParameter& param2 = _modelData.Parameters[ param2Idx ];

                    for( size_t val2Idx = 0; val2Idx < param2.Values.size(); ++val2Idx )
                    {
                        CModelValue& val2 = param2.Values[ val2Idx ];

                        if( !val2.IsPositive() )
                        {
                            Exclusion excl;
                            excl.insert( make_pair( Parameters[ param1Idx ], (int) val1Idx ) );
                            excl.insert( make_pair( Parameters[ param2Idx ], (int) val2Idx ) );
                            Exclusions.insert( excl );
                        }
                    }
                }
            }
        }
    }
}

}
