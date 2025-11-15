namespace PictTests.TestUtil;

internal class CombinationGenerator
{
    public static List<List<DimensionIndex>> GenerateAllCombinations(IList<int> valueCounts, int order)
    {
        var result = new List<List<DimensionIndex>>();
        if (order <= 0 || order > valueCounts.Count) return result;

        void Recurse(int minDim, List<DimensionIndex> current)
        {
            if (current.Count == order)
            {
                result.Add(new List<DimensionIndex>(current));
                return;
            }

            for (var dim = minDim; dim <= valueCounts.Count - (order - current.Count); dim++)
            {
                for (var idx = 0; idx < valueCounts[dim]; idx++)
                {
                    current.Add(new DimensionIndex { Dimension = dim, Index = idx });
                    Recurse(dim + 1, current);
                    current.RemoveAt(current.Count - 1);
                }
            }
        }

        Recurse(0, new List<DimensionIndex>());
        return result;
    }
}
