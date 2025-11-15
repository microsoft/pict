using System.Runtime.InteropServices;

namespace PictNet;

public static class Pict
{
    public static List<List<int>> GenerateIndices(
        IReadOnlyCollection<int> valueCounts,
        uint order = 2,
        uint seed = 0)
    {
        if (valueCounts == null || valueCounts.Count == 0)
            throw new ArgumentException("Must have at least one parameter.", nameof(valueCounts));

        // order currently unused: native helper only does pairwise

        var counts = new UIntPtr[valueCounts.Count];
        var i = 0;
        foreach (var v in valueCounts)
        {
            if (v < 0) throw new ArgumentOutOfRangeException(nameof(valueCounts), "Counts must be non-negative.");
            counts[i++] = (UIntPtr)v;
        }

        var paramCount = (UIntPtr)valueCounts.Count;
        IntPtr cellsPtr;
        UIntPtr rowCountPtr;

        var rc = PictNative.PictGenerateIndices(counts, paramCount, order, seed, out cellsPtr, out rowCountPtr);
        if (rc != 0)
            throw new InvalidOperationException($"PICT generation failed with error code {rc}.");

        try
        {
            var rows64 = rowCountPtr.ToUInt64();
            if (rows64 > int.MaxValue) throw new OverflowException("Too many rows.");

            var rows = (int)rows64;
            var cols = valueCounts.Count;
            var total = rows * cols;

            var flat = new int[total];
            Marshal.Copy(cellsPtr, flat, 0, total);

            var result = new List<List<int>>(rows);
            var k = 0;
            for (var r = 0; r < rows; r++)
            {
                var row = new List<int>(cols);
                for (var c = 0; c < cols; c++)
                    row.Add(flat[k++]);
                result.Add(row);
            }

            return result;
        }
        finally
        {
            if (cellsPtr != IntPtr.Zero)
                PictNative.PictFreeIndices(cellsPtr);
        }
    }
}