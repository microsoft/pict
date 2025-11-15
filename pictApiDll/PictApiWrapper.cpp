// PictApiWrapper.cpp
#include "pch.h"
#include "pictapi.h"
#include <stdlib.h>

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

    // High-level helper that C# calls directly
    __declspec(dllexport)
        int __cdecl PictGenerateIndices(
            const size_t* valueCounts,
            size_t        paramCount,
            unsigned int order,
            unsigned int  randomSeed,
            int** outCells,
            size_t* outRowCount)
    {
        auto task = PictCreateTask();
        if (!task) return PICT_OUT_OF_MEMORY;

        auto model = PictCreateModel(randomSeed);
        if (!model)
        {
            PictDeleteTask(task);
            return PICT_OUT_OF_MEMORY;
        }

        PictSetRootModel(task, model);

        for (size_t i = 0; i < paramCount; ++i)
            PictAddParameter(model, valueCounts[i], order, nullptr);

        auto rc = PictGenerate(task);
        if (rc != PICT_SUCCESS)
        {
            PictDeleteTask(task);
            PictDeleteModel(model);
            *outCells = nullptr;
            *outRowCount = 0;
            return rc;
        }

        PictResetResultFetching(task);
        auto cols = PictGetTotalParameterCount(task);
        auto row = PictAllocateResultBuffer(task);

        size_t rows = 0;
        for (;;)
        {
            if (!PictGetNextResultRow(task, row))
                break;
            ++rows;
        }

        PictResetResultFetching(task);

        auto cells = static_cast<int*>(malloc(rows * cols * sizeof(int)));
        if (!cells)
        {
            PictFreeResultBuffer(row);
            PictDeleteTask(task);
            PictDeleteModel(model);
            *outCells = nullptr;
            *outRowCount = 0;
            return PICT_OUT_OF_MEMORY;
        }

        auto p = cells;
        for (size_t r = 0; r < rows; ++r)
        {
            PictGetNextResultRow(task, row);
            for (size_t c = 0; c < cols; ++c)
                *p++ = row[c];
        }

        PictFreeResultBuffer(row);
        PictDeleteTask(task);
        PictDeleteModel(model);

        *outCells = cells;
        *outRowCount = rows;
        return PICT_SUCCESS;
    }

    __declspec(dllexport)
        void __cdecl PictFreeIndices(int* cells)
    {
        free(cells);
    }
}
