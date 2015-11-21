Pairwise Independent Combinatorial Testing
==========================================

PICT generates test cases and test configurations. With PICT, you can generate tests that are more effective than manually generated tests and in a fraction of the time required by hands-on test case design.

PICT runs as a command line tool. Prepare a model file detailing the parameters of the interface (or set of configurations, or data) you want to test. PICT generates a compact set of parameter value choices that represent the test cases you should use to get comprehensive combinatorial coverage of your parameters.

For instance, if you wish to create a test suite for partition and volume creation, the domain can be described by the following parameters: **Type**, **Size**, **File system**, **Format method**, **Cluster size**, and **Compression**. Each parameter has a limited number of possible values, each of which is determined by its nature (for example, **Compression** can only be **On** or **Off**) or by the equivalence partitioning (such as **Size**).

    Type:          Single, Span, Stripe, Mirror, RAID-5
    Size:          10, 100, 500, 1000, 5000, 10000, 40000
    Format method: Quick, Slow
    File system:   FAT, FAT32, NTFS
    Cluster size:  512, 1024, 2048, 4096, 8192, 16384, 32768, 65536
    Compression:   On, Off

There are thousands of possible combinations of these values. It would be  difficult to test all of them in a reasonable amount of time. Instead, we settle on testing all possible pairs of values. For example, **{Single, FAT}** is one pair, **{10, Slow}** is another; one test case can cover many pairs. Research shows that testing all pairs is an effective alternative to exhaustive testing and much less costly. It will provide very good coverage and the number of test cases will remain manageable.

#More information

See **doc/pict.md** for detailed documentation on PICT and http://pairwise.org has details on this testing methododology. 

#Contributing

PICT consists of three projects:
 * The generator engine,
 * Console PICT.EXE client of the engine,
 * A sample of how the engine API can be used in other projects.

##Building and testing on Windows with MsBuild
Use **pict.sln** to open the solution in Visual Studio 2015. You will need VC++ tools installed.

PICT uses MsBuild for building. **_build.cmd** script in the root directory will build both Debug and Release from the command-line (relies on compilers brought in by VS2015).

The **test** folder contains all that is necessary to test PICT. You need Perl to run the tests. **_test.cmd** is the script that does it all.

The test script produces a log: **dbg.log** or **rel.log** for the Debug and Release bits respectively. Compare them with their committed baselines and make sure all the differences can be explained.

>There are tests which randomize output which typically make it different on each run. These results should be masked in the baseline but currently aren't.

##Building with clang++ on Linux, OS/X, *BSD, etc
Install clang through your package manager (most systems), Xcode (OS/X), or from the [LLVM website](http://llvm.org/releases/).

Run `make` to build the `pict` binary.

Run `make test` to run the tests as described above (requires Perl).
