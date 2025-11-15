using System.Runtime.InteropServices;

namespace PictNet;

public static class Pict
{
    /// <summary>
    /// Given some count of values per index (<paramref name="valueCounts"/>), this generates the minimum full-dimensional combinations to capture all possible combinations of size <paramref name="order"/>.
    /// </summary>
    /// <param name="valueCounts">Each element in this list represents the number of options available for the given dimension</param>
    /// <param name="order">The size of the sub-combinations to be exhausted in the returned combinations. If <paramref name="order"/> = 2, this function will return combinations such that all pairs are present.</param>
    /// <param name="seed">Random seed for reproducibility</param>
    /// <returns>A list of lists. Each sublist represents a specified combination of values. Each element in the sublist represents the index of the parameter at the given dimension.</returns>
    /// <remarks>This is all hard to explain in a short docstring. Please read https://github.com/microsoft/pict</remarks>
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