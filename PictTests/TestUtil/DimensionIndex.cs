namespace PictTests.TestUtil;

internal class DimensionIndex
{
    public int Dimension { get; set; }
    public int Index { get; set; }

    public bool IsContainedIn(IList<int> indices)
    {
        var dimIndex = indices[Dimension];
        return dimIndex == Index;
    }
}

internal static class CombinationExtensions
{
    public static bool ContainsCombination(this IList<int> indices, IEnumerable<DimensionIndex> combination) => combination.All(c => c.IsContainedIn(indices));
}
