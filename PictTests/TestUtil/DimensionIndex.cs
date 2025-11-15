namespace PictTests.TestUtil;

internal class DimensionIndex
{
    public int Dimension { get; set; }
    public int Index { get; set; }

    public bool IsContained(IList<int> indices)
    {
        var dimIndex = indices[Dimension];
        return dimIndex == Index;
    }


}
