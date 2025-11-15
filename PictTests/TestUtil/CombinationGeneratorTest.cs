namespace PictTests.TestUtil;

[TestClass]
public class CombinationGeneratorTest
{

    [TestMethod]
    [DynamicData(nameof(AllPairsCases), DynamicDataSourceType.Method)]
    public void TestAllPairsPresent(List<int> valueCounts, int order, int expectedNumberOfCombos)
    {
        var allCombosOfGivenOrder = CombinationGenerator.GenerateAllCombinations(valueCounts, order);
        Assert.HasCount(expectedNumberOfCombos, allCombosOfGivenOrder);

        var distinctCount = allCombosOfGivenOrder
            .Select(c => string.Join("|", c.OrderBy(c => c).Select(di => $"{di.Dimension}:{di.Index}")))
            .Distinct()
            .Count();

        Assert.AreEqual(expectedNumberOfCombos, distinctCount);
    }

    public static IEnumerable<object[]> AllPairsCases()
    {
        yield return new object[] { new List<int> { 2, 2 }, 2, 4 };
        yield return new object[] { new List<int> { 2, 3 }, 3, 0 };
        yield return new object[] { new List<int> { 3, 3, 3 }, 2, 27 };
        yield return new object[] { new List<int> { 2, 2, 2, 2 }, 2, 24 };
        yield return new object[] { new List<int> { 2, 3, 4 }, 3, 24 };
    }
}
