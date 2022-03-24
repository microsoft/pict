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

# More information

See **[doc/pict.md](https://github.com/Microsoft/pict/blob/master/doc/pict.md)** for detailed documentation on PICT and http://pairwise.org has details on this testing methododology. 

The most recent **pict.exe** is available at https://github.com/microsoft/pict/releases/.

# Contributing

PICT consists of the following projects:
 * **api**: The core combinatorial engine,
 * **cli**: PICT.EXE command-line tool,
 * **clidll**: PICT.EXE client repackaged as a Windows DLL to be used in-proc,
 * **api-usage**: A sample of how the engine API can be used,
 * **clidll-usage**: A sample of how the PICT DLL is to be used.

## Building and testing on Windows with MsBuild
Use **pict.sln** to open the solution in Visual Studio 2019. You will need VC++ build tools installed. See https://www.visualstudio.com/downloads/ for details.

PICT uses MsBuild for building. **_build.cmd** script in the root directory will build both Debug and Release from the command-line.

The **test** folder contains all that is necessary to test PICT. You need Perl to run the tests. **_test.cmd** is the script that does it all.

The test script produces a log: **dbg.log** or **rel.log** for the Debug and Release bits respectively. Compare them with their committed baselines and make sure all the differences can be explained.

>There are tests which randomize output which typically make it different on each run. These results should be masked in the baseline but currently aren't.

## Building on Linux, OS/X, *BSD, etc.
PICT uses CMake to build on Linux.
Assuming installation of CMake and C++ toolchain, following set of commands will build and run tests in the directory `build`
```
> cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
> cmake --build build
> pushd build && ctest -v && popd
```

## Debugging

Most commonly, you will want to debug the command-line tool. Start in the **pictcli** project, **cli/pict.cpp** file. You'll find **wmain** routine there which would be a convenient place to put the very first breakpoint.
