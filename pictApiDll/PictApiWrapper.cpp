// PictApiWrapper.cpp
#include "pch.h"
#include "pictapi.h"

extern "C"
{
    __declspec(dllexport)
        PICT_HANDLE __cdecl PictCreateTask_Wrapper()
    {
        return PictCreateTask();
    }

    __declspec(dllexport)
        PICT_HANDLE __cdecl PictCreateModel_Wrapper(unsigned int randomSeed)
    {
        return PictCreateModel(randomSeed);
    }

    // ...same pattern for the small set of Pict* functions you want to expose
}
