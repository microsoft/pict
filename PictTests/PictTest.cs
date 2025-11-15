using PictNet;

namespace PictTests
{
    [TestClass]
    public sealed class PictTest
    {
        [TestMethod]
        public void GenerateIndices_1_3_3_4_MatchesExpectedSequence()
        {
            var counts = new[] { 1, 3, 3, 4 };

            var rows = Pict.GenerateIndices(counts);

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
        }

        private void AssertAllPairsPresent()
    }
}
