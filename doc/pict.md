PICT
========
Pairwise Independent Combinatorial Testing tool
------------------------------------------------
jacekcz@microsoft.com

PICT generates test cases and test configurations. With PICT, you can generate tests that are more effective than manually generated tests and in a fraction of the time required by hands-on test case design.

PICT runs as a command line tool. Prepare a model file detailing the parameters of the interface (or a set of configurations, or data) you want to test and PICT generates a compact set of parameter value choices that represent the test cases you should use to get comprehensive combinatorial coverage of your parameters.

For instance, to create a test suite for disk partition creation, the domain can be described by the following parameters: ```Partition Type```, ```Partition Size```, ```File System```, ```Format Method```, ```Cluster Size```, and ```Compression```. Each parameter consists of a finite number of possible values. For example, ```Compression``` can only be ```On``` or ```Off```, other parameters are made finite with help of equivalence partitioning:

    Type:          Single, Span, Stripe, Mirror, RAID-5
    Size:          10, 100, 500, 1000, 5000, 10000, 40000
    Format method: Quick, Slow
    File system:   FAT, FAT32, NTFS
    Cluster size:  512, 1024, 2048, 4096, 8192, 16384, 32768, 65536
    Compression:   On, Off

For such a model, thousands of possible test cases can be generated. It would be difficult to test all of them in a reasonable amount of time. Instead of attempting to cover all possible combinations, we settle on testing all possible pairs of values. For example, ```{Type:Single, File system:FAT}``` is one such value pair, ```{Size:10, Format method:Slow}``` is another. Consequently, one test case can cover many pairs. Research shows that testing all pairs is an effective alternative to exhaustive testing and is much less costly.

# Usage

PICT is a command-line tool that accepts a plain-text model file as an input and produces a set of test cases.

    Usage: pict model [options]

    Options:
      /o:N|max - Order of combinations (default: 2)
      /d:C     - Separator for values  (default: ,)
      /a:C     - Separator for aliases (default: |)
      /n:C     - Negative value prefix (default: ~)
      /e:file  - File with seeding rows
      /r[:N]   - Randomize generation, N - seed
      /c       - Case-sensitive model evaluation
      /s       - Show model statistics

## Model File 

A model is a plain-text file consisting of the following sections:

    parameter definitions
    [sub-model definitions]
    [constraint definitions]
 
The sections should always be specified in the order shown above. All parameters should be defined first, then sub-models, and then constraints. Parameter definitions are required, other parts are optional. Sections do not require any special separators between them. Empty lines can appear anywhere. Comments are permitted and they should be prefixed with # character.

To produce a basic model file, list the parameters—each in a separate line—with the respective values delimited by commas:

    <ParamName> : <Value1>, <Value2>, <Value3>, ...

Example:

    #
    # This is a sample model for testing volume creation
    #

    Type:          Primary, Logical, Single, Span, Stripe, Mirror, RAID-5
    Size:          10, 100, 500, 1000, 5000, 10000, 40000
    Format method: quick, slow
    File system:   FAT, FAT32, NTFS
    Cluster size:  512, 1024, 2048, 4096, 8192, 16384, 32768, 65536
    Compression:   on, off

A comma is the default separator but you can specify a different one using ```/d``` option.

By default, PICT generates a pair-wise test suite (all pairs covered), but the order can be set by option ```/o``` to a value larger than two. For example, when ```/o:3``` is specified, the test suite will cover all triplets of values. It will produce a larger number of tests but will potentially make the test suite even more effective. The maximum order for a model is equal to the number of parameters, which will result in an exhaustive test suite. Following the same principle, specifying ```/o:1``` will produce a test suite that merely covers all values (combinations of 1).

## Output Format

The test cases are printed to the standard output stream. The first line of the output contains names of the parameters. Each of the following lines represents one generated test case. Values in each line are separated by a tab. This way redirecting the output to a file creates a tab-separated value format.

If a model and options given to the tool do not change, every run will result in the same output. However, the output can be randomized if ```/r``` option is used. A randomized generation prints out the seed used for that particular execution to the error output stream. Consequently, that seed can be fed into the tool with ```/r:seed``` option to replay a particular generation.

Different random seed values will often produce a different number of total test cases.  This is because packing n-way combinations is a “hard problem” for which PICT and other tools use heuristics.  These heuristics are deterministic, but they are dependent on initial conditions. Sometimes the algorithm is lucky and packs all of your desired combinations into fewer test cases.  Variations of 5% - 10% are common.

All errors, warning messages, and other auxiliary information is printed to the error stream.

# Constraints

Constraints express inherent limitations of the modelled domain. In the example above, one of the pairs that will appear in at least one test case is ```{File system:FAT, Size:5000}```. In practice, the FAT file system cannot be applied on volumes larger than 4,096 MB. Note that you cannot simply remove those violating test cases from the result because an offending test case may cover other, possibly valid, pairs that would not otherwise be tested. Instead of losing valid pairs, it is better to eliminate disallowed combinations during the generation process. In PICT, this can be done by specifying constraints, for example:

    Type:           Primary, Logical, Single, Span, Stripe, Mirror, RAID-5
    Size:           10, 100, 500, 1000, 5000, 10000, 40000
    Format method:  quick, slow
    File system:    FAT, FAT32, NTFS
    Cluster size:   512, 1024, 2048, 4096, 8192, 16384, 32768, 65536
    Compression:    on, off

    IF [File system] = "FAT"   THEN [Size] <= 4096;
    IF [File system] = "FAT32" THEN [Size] <= 32000;

## Conditional Constraints

A term ```[parameter] relation value``` is an atomic part of a constraint expression. The following relations can be used: ```=```, ```<>```, ```>```, ```>=```, ```<```, ```<=```, and ```LIKE```. ```LIKE``` is a wildcard-matching string operator (```*``` - any character, ```?``` – one character).

    [Size] < 10000
    [Compression] = "OFF"
    [File system] LIKE "FAT*"

Operator IN allows specifying a set of values:

    IF [Cluster size] IN {512, 1024, 2048} THEN [Compression] = "Off";
    IF [File system] IN {"FAT", "FAT32"}   THEN [Compression] = "Off";

The ```IF```, ```THEN```, and ```ELSE``` parts of an expression may contain multiple terms joined by logical operators: ```NOT```, ```AND```, and ```OR```. Parentheses are allowed in order to change the default operator priority:

    IF [File system] <> "NTFS" OR
     ( [File system] =  "NTFS" AND [Cluster size] > 4096 ) 
    THEN [Compression] = "Off";

    IF NOT ( [File system] = "NTFS" OR 
           ( [File system] = "NTFS" AND NOT [Cluster size] <= 4096 )) 
    THEN [Compression] = "Off";

Parameters can be compared to other parameters, like in this example:

    #
    # Machine 1
    #
    OS_1:   Win7, Win8, Win10
    SKU_1:  Home, Pro
    LANG_1: English, Spanish, Chinese

    #
    # Machine 2
    #
    OS_2:   Win7, Win8, Win10
    SKU_2:  Home, Pro
    LANG_2: English, Spanish, Chinese, Hindi

    IF [LANG_1] = [LANG_2]
    THEN [OS_1] <> [OS_2] AND [SKU_1] <> [SKU_2];

## Unconditional Constraints (Invariants)

An invariant declares an always valid limitation of a domain:

    #
    # At least one parameter must be different to be a meaningful test case
    #

    [OS_1] <> [OS_2] OR [SKU_1] <> [SKU_2] OR [LANG_1] <> [LANG_2];

    #
    # All parameters must be different (we use AND operator)
    #

    [OS_1] <> [OS_2] AND [SKU_1] <> [SKU_2] AND [LANG_1] <> [LANG_2];

## Types

PICT has a simple type system. There are two types of parameters: a string and a numeric type. Types do not have to be explicitly declared. A parameter is considered numeric when all of its values can be converted to a number (an integer or a float).

 Types are only important when evaluating constraints. A numeric parameter is only comparable to a number, and a string parameter is only comparable to another string. For example:

    Size:  1, 2, 3, 4, 5
    Value: a, b, c, d

    IF [Size] > 3 THEN [Value] > "b";

If a value has multiple names, only the first is considered when PICT detects the type.

## Case Sensitiveness

By default, PICT does all its comparisons and checks case-insensitively. For instance, if there are two parameters defined: ```OS``` and ```os```, a duplication of names will be detected (parameter names must be unique). Constraints are also resolved case-insensitively by default:

    IF [OS] = "Win10" THEN ...

will match both ```Win10``` and ```win10``` values (values of a parameter are not required to be unique). Option ```/c``` however, makes the model evaluation fully case-sensitive.

# Advanced Modelling Features

## Re-using Parameter Definitions

Once a parameter is defined, it can help in defining other parameters.

    #
    # Machine 1
    #
    OS_1:   Win7, Win8, Win10
    SKU_1:  Home, Pro
    LANG_1: English, Spanish, Chinese

    #
    # Machine 2
    #
    OS_2:   <OS_1>
    SKU_2:  <SKU_1>
    LANG_2: <LANG_1>, Hindi

Less typing and better for maintainability.
    
## Sub-Models

Sub-models allow the bundling of certain parameters into groups that get their own combinatory orders. This can be useful if combinations of certain parameters need to be tested more thoroughly, or less thoroughly, or in separation from the other parameters in the model. The sub-model definition has the following format:

    { <ParamName1>, <ParamName2>, <ParamName3>, ... } @ <Order>

For example, sub-modeling is useful when hardware and software parameters are combined together. Without sub-models, each test case would produce a new, unique (and costly) hardware configuration. Placing all hardware parameters into one sub-model produces fewer unique hardware configurations and so potentially lowers the cost of testing.  The order of combinations that can be assigned to each sub-model allows for additional flexibility.

    PLATFORM:  x86, x64, arm
    CPUS:      1, 2, 4
    RAM:       1GB, 4GB, 64GB
    HDD:       SCSI, IDE
    OS:        Win7, Win8, Win10
    Browser:   Edge, Opera, Chrome, Firefox
    APP:       Word, Excel, Powerpoint
    
    { PLATFORM, CPUS, RAM, HDD } @ 2

The test generation for the above model would proceed as follows:

                                     $
                                     |
                                     | order = 3 (defined by /o:3) 
                                     |
                +--------------------+-------------------+
                |                                        |
                | order = 2                              | order = 3 
                |                                        |
    { PLATFORM, CPUS, RAM, HDD }                  OS, Browser, APP 

For the above model with sub-model, PICT invoked with an /o:3 argument will generate roughly 10 hardware configurations, compared to roughly 40 hardware configurations without the sub-model.  This reduced number of hardware configurations comes at the cost of a greater number of total test cases: roughly 140 test cases with the sub-model, compared to roughly 60 test cases without the sub-model (again, all with /o:3).

Notes:
 1. You can define as many sub-models as you want; any parameter can belong to any number of sub-models. However, the model hierarchy can be just one level deep. 
 2. The combinatory order of a sub-model cannot exceed the number of its parameters. In the example above, an order of the first sub-model can be any value between one and four. 
 3. If you do not specify the order for a sub-model, the order specified by ```/o``` option will be used.

## Aliasing

Aliasing is a way of specifying multiple names for a single value. Multiple names do not change the combinatorial complexity of the model. No matter how many names a value has, they are treated as one entity. The only difference will be in the output; any test case that would normally have that one value will have one of its names instead. Names are rotated among the test cases.

By default, names should be separated by | character but this can be changed with option ```/a```.

    OS_1:   Win2008, Win2012, Win2016
    SKU_1:  Professional, Server | Datacenter

Note:
 1. When evaluating constraints, only the first name counts. For instance, ```[SKU_1] = "Server"``` will result in a match but ```[SKU_1] = "Datacenter"``` will not. Also, only the first name is used to determine whether a value is "negative" or for the purpose of detecting its type.

## Negative Testing

In addition to testing valid combinations, referred to as “positive testing,” it is often desirable to test using values outside the allowable range to make sure the program handles errors properly. This “negative testing” should be conducted such that only one out-of-range value is present in any test case. This is because of the way in which typical applications are written: namely, they often error out when the first error is detected.

"Input masking" is a type of problem in test design, in which one out-of-range input prevents another invalid input from being tested.

Consider the following routine, which takes two arguments:

    float SumSquareRoots( float a, float b )
    {
          if ( a < 0 ) throw error;           // [1]
          if ( b < 0 ) throw error;           // [2]

          return ( sqrt( a ) + sqrt( b ));
    };


Although the routine can be called with any numbers for ```a``` or ```b```, it only makes sense to do the calculation on non-negative numbers. For that reason by the way, the routine does verifications [1] and [2] on the arguments. Now, assume a test ```( a: -1, b: -1 )``` was used to test that negative case. Here, ```a: -1``` "masks" ```b: -1``` because the check [2] never gets executed. If it did not exist at all, that problem in code would go unnoticed.

To prevent input masking issues, it is important that two out-of-range values do not appear in the same test case. Prefixing a value with ```~``` (tilde) marks it invalid or out-of-allowable-range. Option ```/n``` allows for specifying a different prefix.

    #
    # SumSquareRoots model
    #

    A: ~-1, 0, 1, 2
    B: ~-1, 0, 1, 2

PICT guarantees that all possible pairs of valid (or in-range) values will be covered and all possible combinations of any out-of-range / invalid value will be paired only with all valid values.

    A       B
    2       0
    2       2
    0       0
    0       1
    1       0
    2       1
    0       2
    1       2
    1       1
    ~-1     2
    1       ~-1
    0       ~-1
    ~-1     1
    2       ~-1
    ~-1     0

Notes:
 1. The ```~``` prefix is not a part of a value when it comes to type detection or constraint evaluation. A valid constraint for the above example would look like this: ```if [A] = -1 then [B] = 0;```. The ```~``` prefix however will show up in the output.
 2. If a value has multiple names, only prefixing the first name will make the value out-of-range. 
 
## Excluding Entire Parameters - The "Dummy Value Technique"

Some test suites may require that under certain conditions entire parameters should be ignored. Negative testing (as described above) could be such a scenario. There, an error or a failure may stop the application under test and make any other parameter choices meaningless. By design, PICT will try to pack as many combinations into the fewest test cases possible, including those that contain values that will trigger failures.  In these cases, we will see testable pairs essentially wasted.  In some scenarios the ```~``` prefix can be used to prevent this, but for scenarios where it doesn't apply, we can use the "dummy value technique" instead.

PICT does not have any special handling for the dummy value technique.  To use it, you need to manually modify your model as described below.

Consider the following example where a ```P1: -1``` is a value that triggers an application error. 

    P1: -1, 0, 1
    P2:  A, B, C
    P3:  X, Y, Z
    
We want to test this condition independently and not associate any values of ```P2``` or ```P3``` with it. To accomplish that, we add a dummy value ```NA``` to parameters ```P2``` and ```P3```. That dummy value will indicate that the parameter should be ignored. We can then use constraints to make sure tests are generated correctly:

    P1: -1, 0, 1
    P2:  A, B, C, NA
    P3:  X, Y, Z, NA
    
    IF [P1] = -1  
      THEN [P2] =  "NA" AND [P3] =  "NA" 
      ELSE [P2] <> "NA" AND [P3] <> "NA";

This will result in a single test for ```P1: -1``` with ```NA``` set for ```P2``` and ```P3```. No other combinations values of ```P2``` and ```P3``` are used up for the test containing ```P1: -1```:

    P1      P2      P3
    0       C       Z
    1       B       Z
    1       C       Y
    0       A       X
    1       C       X
    0       B       X
    0       A       Y
    -1      NA      NA
    0       B       Y
    1       A       Z

The downside of this method is that constraints will get progressively more complex. For example, when additional failure inducing values are added to the model e.g. ```P2: Null```:

    P1: -1, 0, 1
    P2: A, B, C, Null, NA
    P3: X, Y, Z, NA
    
    IF [P1] = -1
      THEN [P2] = "NA" AND [P3] = "NA"
      ELSE [P2] <> "NA" AND 
         ( [P3] <> "NA" OR [P2] = "Null" );
      
    IF [P2] = "Null" OR [P2] = "NA" 
      THEN [P3] = "NA";

Two additional tests will be added (not all output shown):

    P1      P2      P3
    0       Null    NA
    1       Null    NA
		
A more concrete example: 

    OS: Windows, Ubuntu
    CPU: Intel, AMD
    DBMS: PostgreSQL, MySQL
    JavaVersion: 18, 19, 20
    DotNetVersion: 4.8, 4.8.1

The above model will generate these tests:
    
    OS          CPU     DBMS        JavaVersion     DotNetVersion
    Windows     AMD     PostgreSQL  18              4.8
    Windows     Intel   MySQL       20              4.8.1
    Ubuntu      Intel   PostgreSQL  19              4.8.1
    Ubuntu      AMD     MySQL       20              4.8
    Windows     AMD     MySQL       19              4.8
    Ubuntu      AMD     MySQL       18              4.8.1
    Windows     Intel   MySQL       18              4.8
    Ubuntu      Intel   PostgreSQL  20              4.8

The problem is that there is no .NET on Ubuntu, so the DotNetVersion parameter doesn't make any sense for those tests.  If we add "NA" and a constraint to the model, like this:

    OS: Windows, Ubuntu
    CPU: Intel, AMD
    DBMS: PostgreSQL, MySQL
    JavaVersion: 18, 19, 20
    DotNetVersion: 4.8, 4.8.1, NA
    
    IF [OS] = "Ubuntu"
      THEN [DotNetVersion] = "NA"
      ELSE [DotNetVersion] <> "NA";

... then the generated tests will be these:

    OS          CPU     DBMS        JavaVersion     DotNetVersion
    Windows     AMD     PostgreSQL  20              4.8
    Ubuntu      Intel   MySQL       18              NA
    Windows     Intel   PostgreSQL  18              4.8
    Windows     AMD     MySQL       18              4.8.1
    Windows     AMD     MySQL       19              4.8
    Ubuntu      AMD     PostgreSQL  19              NA
    Windows     Intel   PostgreSQL  19              4.8.1
    Windows     Intel   MySQL       20              4.8.1
    Ubuntu      AMD     PostgreSQL  20              NA


## Weighting

Weights tell the generator to prefer certain parameter values over others. Weights are positive integers. When not explicitly specified, values have a weight of 1:

    #
    # Let’s focus on primary partitions formatted with NTFS
    #

    Type:           Primary (10), Logical, Single, Span, Stripe, Mirror, RAID-5
    Format method:  quick, slow
    File system:    FAT, FAT32, NTFS (10)

Weights are not guarantees but merely opportunistic hints. You cannot be sure that the weights you specified will be honored at all.

PICT's main objective is to cover all combinations of values in the smallest number of test cases. Only when value choices do not affect that coverage criterion, weights will be used to determine the final result.

For that reason, weight values have no intuitive meaning. For example, when a parameter is defined like this:

    File system:    FAT, FAT32, NTFS (10)

It does not mean that NTFS will appear in the output ten times more often than FAT or FAT32. It will merely be 10 times more likely to be chosen when the choice does not affect the coverage.

## Seeding

Seeding makes two scenarios possible:
 1. Allows for specifying important e.g. regression-inducing combinations that should end up in any generated test suite. In this case, PICT will initialize the output with combinations you provide and will build up the rest of the suite on top of those. 
 2. Minimize changes to the output when your model is modified. You can feed PICT the results of a previous test generation and expect the tool to reuse the old test cases as much as possible.

Seeding rows must be defined in a separate file. Use option ```/e``` to specify the seeding file's location:

    pict.exe model.txt /e:seedrows.txt 

Seeding files have the same format as any PICT output. The first line contains parameter names separated by tab characters and each following line contains one test case with values also separated by tabs. This format can easily be prepared manually or prior output can be directly fed back to PICT:

    Ver      SKU   Lang  Arch
    Win7     Pro   EN    x86 
    Win7           FR    x86 
    Win10    Pro   EN    x64

A seeding row may be complete i.e. have values specified for all parameters. It can also be partial, e.g. the second seeding row in the example above does not have a value for the ```SKU``` parameter. In this case, the generation process will take care of choosing the best value for ```SKU```.

There are a few rules of matching seeding rows with the current model:
 1. If a seeding file contains a parameter that is not in the current model, the entire column representing the parameter in the seeding file will be ignored. Parameters are matched by name.
 2. If a seeding row contains a value that is not in the current model, the value will be ignored. The rest of the row however, if valid, will still be used. As a result of that, a complete row may become a partial row.
 3. If a seeding row violates any of the current constraints, it will be skipped entirely.

PICT will issue warnings if (1) or (2) occurs.

In addition, there are a few things that are normally allowed in PICT models but may lead to ambiguities when seeding is used:
 1. Blank parameter and value names.
 2. Parameter and value names containing tab characters.
 3. Values and all their aliases not unique within a parameter. 

These are best to avoid in any case. When seeding is used, you will be warned if any of the above problems are detected in your model.

# Constraints Grammar

    Constraints   :: =
      Constraint
    | Constraint Constraints

    Constraint    :: =
      IF Predicate THEN Predicate ELSE Predicate;
    | Predicate;

    Predicate     :: =
      Clause
    | Clause LogicalOperator Predicate

    Clause        :: =
      Term
    | ( Predicate )
    | NOT Predicate

    Term          :: =
      ParameterName Relation Value
    | ParameterName LIKE PatternString
    | ParameterName IN { ValueSet }
    | ParameterName Relation ParameterName

    ValueSet       :: =
      Value
    | Value, ValueSet

    LogicalOperator ::=
      AND 
    | OR

    Relation      :: = 
      = 
    | <> 
    | >
    | >=
    | <
    | <=

    ParameterName ::= [String]

    Value         :: =
      "String"
    | Number

    String        :: = whatever is typically regarded as a string of characters

    Number        :: = whatever is typically regarded as a number

    PatternString ::= string with embedded special characters (wildcards):
                      * a series of characters of any length (can be zero)
                      ? any one character
