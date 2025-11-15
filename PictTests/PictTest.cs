using PictNet;
using PictTests.TestUtil;

namespace PictTests
{
    [TestClass]
    public sealed class PictTest
    {
        [TestMethod]
        public void GenerateIndices_1_3_3_4_MatchesExpectedSequence()
        {
            var counts = new[] { 1, 3, 3, 4 };

            var rows = Pict.GenerateIndices(counts, 2);

            var actual = new List<int>(rows.Count * counts.Length);
            foreach (var row in rows)
            {
                Assert.AreEqual(counts.Length, row.Count, "Unexpected column count in a row.");
                actual.AddRange(row);
            }

            var expected = new List<int>
            {
                0, 0, 1, 1,
                0, 1, 0, 2,
                0, 2, 0, 0,
                0, 2, 2, 1,
                0, 1, 1, 0,
                0, 1, 0, 1,
                0, 2, 1, 2,
                0, 0, 1, 3,
                0, 1, 0, 3,
                0, 0, 2, 0,
                0, 1, 2, 2,
                0, 2, 2, 3,
                0, 0, 0, 2
            };

            CollectionAssert.AreEqual(expected, actual);

            AssertAllPairsPresent(rows, counts, 2);
        }


        [TestMethod]
        [DynamicData(nameof(AllPairsCases), DynamicDataSourceType.Method)]
        public void TestAllPairsPresent(List<int> valueCounts, int order)
        {
            var rows = Pict.GenerateIndices(valueCounts, (uint)order);
            AssertAllPairsPresent(rows, valueCounts, order);
        }

        public static IEnumerable<object[]> AllPairsCases()
        {
            yield return new object[] { new List<int> { 2, 2 }, 2 };
            yield return new object[] { new List<int> { 2, 3 }, 3 };
            yield return new object[] { new List<int> { 3, 3, 3 }, 2 };
            yield return new object[] { new List<int> { 2, 2, 2, 2 }, 2 };
            yield return new object[] { new List<int> { 2, 3, 4 }, 3 };
        }


        private void AssertAllPairsPresent(List<List<int>> rows, IList<int> valueCounts, int order)
        {
            var combinations = CombinationGenerator.GenerateAllCombinations(valueCounts, order);
            foreach (var combo in combinations)
            {
                Assert.IsTrue(rows.Any(row => row.ContainsCombination(combo)));
            }
        }
    }
}
