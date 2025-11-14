// PictSimpleWrapper.cpp
#include "pictapi.h"
#include <stdlib.h>

extern "C"
{
    // valueCounts: array of length paramCount (e.g. {1,3,3,4})
    // outCells:    flattened [rowCount x paramCount] table of indices (0-based)
    //              caller frees with PictFreeIndices
    __declspec(dllexport)
        PICT_RET_CODE __cdecl PictGenerateIndices(
            const size_t* valueCounts,
            size_t        paramCount,
            unsigned int  randomSeed,
            int** outCells,
            size_t* outRowCount)
    {
        auto task = PictCreateTask();
        if (!task) return PICT_OUT_OF_MEMORY;

        auto model = PictCreateModel(randomSeed);
        if (!model) { PictDeleteTask(task); return PICT_OUT_OF_MEMORY; }

        PictSetRootModel(task, model);

        for (size_t i = 0; i < paramCount; ++i)
            PictAddParameter(model, valueCounts[i], 0, nullptr);

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
            if (!PictGetNextResultRow(task, row)) break;
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
                *p++ = row[c];      // PICT gives you value indices here (typically 0..valueCount-1)
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
