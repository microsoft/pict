PICT
========
Pairwise Independent Combinatorial Testing tool
------------------------------------------------
jacekcz@microsoft.com

PICT generates test cases and test configurations. With PICT, you can generate tests that are more effective than manually generated tests and in a fraction of the time required by hands-on test case design.

PICT runs as a command line tool. Prepare a model file detailing the parameters of the interface (or set of configurations, or data) you want to test. PICT generates a compact set of parameter value choices that represent the test cases you should use to get comprehensive combinatorial coverage of your parameters.

For instance, to create a test suite for disk partition creation, the domain can be described by the following parameters: **Type**, **Size**, **File system**, **Format method**, **Cluster size**, and **Compression**. Each parameter consists of a finite number of possible values. For example, **Compression** naturally can only be **On** or **Off**, other parameters are made finite with help of equivalence partitioning (e.g. **Size**).

    Type:          Single, Span, Stripe, Mirror, RAID-5
    Size:          10, 100, 500, 1000, 5000, 10000, 40000
    Format method: Quick, Slow
    File system:   FAT, FAT32, NTFS
    Cluster size:  512, 1024, 2048, 4096, 8192, 16384, 32768, 65536
    Compression:   On, Off

For such a model, thousands of possible test cases can be generated. It would be difficult to test all of them in a reasonable amount of time. Instead of attempting to cover all possible combinations, we settle on testing all possible pairs of values. For example, **{Single, FAT}** is one pair, **{10, Slow}** is another. Consequently, one test case can cover many pairs. Research shows that testing all pairs is an effective alternative to exhaustive testing and much less costly. It will provide very good coverage and the number of test cases will remain manageable.

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

A model consists of the following sections:

    parameter definitions
    [sub-model definitions]
    [constraint definitions]
 
Model sections should always be specified in the order shown above and cannot overlap. All parameters should be defined first, then sub-models, and then constraints. The sub-models definition section is optional, as well as the constraints. Sections do not require any special separators between them. Empty lines can appear anywhere. Comments are also permitted and they should be prefixed with # character.

To produce a basic model file, list the parameters—each in a separate line—with the respective values delimited by commas:

    <ParamName> : <Value1>, <Value2>, <Value3>, ...

Example:

    #
    # This is a sample model for testing volume create/delete functions
    #

    Type:          Primary, Logical, Single, Span, Stripe, Mirror, RAID-5
    Size:          10, 100, 500, 1000, 5000, 10000, 40000
    Format method: quick, slow
    File system:   FAT, FAT32, NTFS
    Cluster size:  512, 1024, 2048, 4096, 8192, 16384, 32768, 65536
    Compression:   on, off

A comma is the default separator but you can specify a different one using **/d** option.

By default, PICT generates a pair-wise test suite (all pairs covered), but the order can be set by option **/o** to a value larger than two. For example, if **/o:3** is specified, the test suite will cover all triplets of values thereby producing a larger number of tests but potentially making the test suite even more effective. The maximum order for a simple model is equal to the number of parameters, which will result in an exhaustive test suite. Following the same principle, specifying **/o:1** will produce a test suite that merely covers all values (combinations of 1).

## Output Format

All errors, warning messages, and the randomization seed are printed to the error stream. The test cases are printed to the standard output stream. The first line of the output contains names of the parameters. Each of the following lines represents one generated test case. Values in each line are separated by a tab. This way redirecting the output to a file creates a tab-separated value format.

If a model and options given to the tool do not change, every run will result in the same output. However, the output can be randomized if **/r** option is used. A randomized generation prints out the seed used for that particular execution to the error output stream. Consequently, that seed can be fed into the tool with **/r:seed** option to replay a particular generation.

# Constraints

Constraints allow you to specify limitations on the domain. In the example with partitions, one of the pairs that will occur in at least one test case is **{FAT, 5000}**. In reality, the FAT file system cannot be applied on volumes larger than 4,096 MB. Note that you cannot simply remove those violating test cases from the result because an offending test case may cover other, possibly valid, pairs that would not otherwise be tested. Instead of losing valid pairs, it is better to eliminate disallowed combinations during the generation process. In PICT, this can be done by specifying constraints, for example:

    Type:           Primary, Logical, Single, Span, Stripe, Mirror, RAID-5
    Size:           10, 100, 500, 1000, 5000, 10000, 40000
    Format method:  quick, slow
    File system:    FAT, FAT32, NTFS
    Cluster size:   512, 1024, 2048, 4096, 8192, 16384, 32768, 65536
    Compression:    on, off

    IF [File system] = "FAT"   THEN [Size] <= 4096;
    IF [File system] = "FAT32" THEN [Size] <= 32000;

## Conditional Constraints

A term **[parameter] relation value** is an atomic part of a constraint expression. The following relations can be used: =, <>, >, >=, <, <=, and LIKE. LIKE is a wildcard-matching operator (* - any character, ? – one character).

    [Size] < 10000
    [Compression] = "OFF"
    [File system] like "FAT*"

Operator IN allows specifying a set of values that satisfy the relation explicitly:

    IF [Cluster size] in {512, 1024, 2048} THEN [Compression] = "Off";
    IF [File system] in {"FAT", "FAT32"}   THEN [Compression] = "Off";

The IF, THEN, and ELSE parts of an expression may contain multiple terms joined by logical operators: NOT, AND, and OR. Parentheses are also allowed in order to change the default operator priority:

    IF [File system] <> "NTFS" OR
     ( [File system] =  "NTFS" AND [Cluster size] > 4096 ) 
    THEN [Compression] = "Off";

    IF NOT ( [File system] = "NTFS" OR 
           ( [File system] = "NTFS" AND NOT [Cluster size] <= 4096 )) 
    THEN [Compression] = "Off";

Parameters can also be compared to other parameters, like in this example:

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

    [OS_1] <> [OS_2] or [SKU_1] <> [SKU_2] or [LANG_1] <> [LANG_2];

    #
    # All parameters must be different (we use AND operator)
    #

    [OS_1] <> [OS_2] and [SKU_1] <> [SKU_2] and [LANG_1] <> [LANG_2];

## Types

PICT uses the concept of a parameter type. There are two types of parameters: string and numeric. A parameter is considered numeric only when all its values are numeric. If a value has multiple names, only the first one counts. Types are only important when evaluating constraints. A numeric parameter is only comparable to a number, and a string parameter is only comparable to another string. For example:

    Size:  1, 2, 3, 4, 5
    Value: a, b, c, d

    IF [Size] > 3 THEN [Value] > "b";

String comparison is lexicographical and case-insensitive by default. Numerical values are compared as numbers.

## Case Sensitiveness

By default, PICT does all its comparisons and checks case-insensitively. For instance, if there are two parameters defined: *OS* and *os*, a duplication of names will be detected (parameter names must be unique). Constraints are also resolved case-insensitively by default:

    IF [OS] = "Win10" THEN ...

will match both *Win10* and *win10* values (values of a parameter are not required to be unique). Option **/c** however, makes the model evaluation fully case-sensitive.

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

Less typing and better maintainability. 
    
## Sub-Models

Sub-models allow the bundling of certain parameters into groups that get their own combinatory orders. This can be useful if combinations of certain parameters need to be tested more thoroughly or must be combined in separation from the other parameters in the model. The sub-model definition has the following format:

    { <ParamName1>, <ParamName2>, <ParamName3>, ... } @ <Order>

For example, sub-modeling is useful when hardware and software parameters are combined together. Without sub-models, each test case would produce a new, unique hardware configuration. Placing all hardware parameters into one sub-model produces fewer distinct hardware configurations and potentially lowers the cost of testing. The order of combinations that can be assigned to each sub-model allows for additional flexibility.

    PLATFORM:  x86, x64, arm
    CPUS:      1, 2, 4
    RAM:       1GB, 4GB, 64GB
    HDD:       SCSI, IDE
    OS:        Win7, Win8, Win10
    Browser:   Edge, Opera, Chrome, Firefox
    APP:       Word, Excel, Powerpoint
    
    { PLATFORM, CPUS, RAM, HDD } @ 3
    { OS, Browser } @ 2

The test generation for the above model would proceed as follows:

                                               $
                                               |
                                               | order = 2 (defined by /o) 
                                               |
                +------------------------------+-----------------------------+
                |                              |                             |
                | order = 3                    | order = 2                   |
                |                              |                             |
    { PLATFORM, CPUS, RAM, HDD }        { OS, Browser }                     APP 

Notes:
 1. You can define as many sub-models as you want; any parameter can belong to any number of sub-models. Model hierarchy can be just one level deep. 
 2. The combinatory order of a sub-model cannot exceed the number of its parameters. In the example above, an order of the first sub-model can be any value between one and four. 
 3. If you do not specify the order for a sub-model, the default order, as specified by /o option, will be used. 

## Aliasing

Aliasing is a way of specifying multiple names for a single value. Multiple names do not change the combinatorial complexity of the model. No matter how many names a value has, it is treated as one entity. The only difference will be in the output; any test case that would normally have that one value will have one of its names instead. Names are rotated among the test cases. Specifying one value with two names will result in having them both show up in the output without additional test cases.

By default, names should be separated by | character but this can be changed with option **/a**.

    OS_1:   Win2008, Win2012, Win2016
    SKU_1:  Professional, Server | Datacenter

Note:
When evaluating constraints, only the first name counts. For instance, **[SKU_1] = "Server"** will result in a match but **[SKU_1] = "Datacenter"** will not. Also, only the first name is used to determine whether a value is negative or a numeric type.

## Negative Testing

In addition to testing valid combinations, referred to as “positive testing,” it is often desirable to test using values outside the allowable range to make sure the program handles errors properly. This “negative testing” should be conducted such that only one invalid value is present in any test case. This is due to the way in which typical applications are written: namely, to take some failure action upon the first error detected. For this reason, a problem known as input masking—in which one invalid input prevents another invalid input from being tested—can occur with negative testing.

Consider the following routine, which takes two arguments:

    float SumSquareRoots( float a, float b )
    {
          if ( a < 0 ) throw error;           // [1]
          if ( b < 0 ) throw error;           // [2]

          return ( sqrt( a ) + sqrt( b ));
    };


Although the routine can be called with any numbers a or b, it only makes sense to do the calculation on non-negative numbers. For that reason by the way, the routine does verifications [1] and [2] on the arguments. Now, assume a test ( a= -1, b = -1 ) was used to test a negative case. Here, a = -1 actually masks b = -1 because check [2] never gets executed and it would go unnoticed if it didn’t exist. 


To prevent input masking, it is important that two invalid values (of two different parameters) are not in the same test case. Prefixing any value with ~ (tilde) marks it invalid. Option **/n** allows for specifying a different prefix.

    #
    # Trivial model for SumSquareRoots
    #

    A: ~-1, 0, 1, 2
    B: ~-1, 0, 1, 2

The tool guarantees that all possible pairs of valid values will be covered and all possible combinations of any invalid value will be paired with all positive values at least once.

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
 1. A prefix is not a part of a value when it comes to comparisons therefore in constraints it should appear without it. A valid constraint (although quite artificial in this example) would be: **if [A] = -1 then [B] = 0;**. Also checking for the type of a value is not affected by the prefix. For example, both parameters in the above example are numeric despite having non-numeric “~” in one of their values. The prefix however will show up in the output.  
 2. If a value has multiple names, only prefixing the first name will make the value negative. 
 
### Excluding other parameters
Sometimes we may want to ignore other parameters for certain values. A negative test is a typical scenario, as a failure may make any other parameters meaningless if it stops the application. In these cases we do not want to 'use up' any testable values for other parameters. A technique to handle this is be to add a dummy value to the parameters to be excluded. Since it is a dummy value, it must in turn be excluded from the other tests, which can be accomplished using the ELSE clause. 

Consider the following example where a `P1` value of `-1` is a negative test where we expect the application to fail. 

    P1: -1, 0, 1
    P2:  A, B, C
    P3:  X, Y, Z
    
We want to test this independently and not associate any `P2` and `P3` values with it, as it would needlessly increase the amount of tests and use up `P2`/`P3` combinations. So we add a dummy value `NA` to the `P2` and `P3` sets and exclude them from other cases with the ELSE clause:

    P1: -1, 0, 1
    P2:  A, B, C, NA
    P3:  X, Y, Z, NA
    
    IF [P1] = -1  
      THEN [P2] =  "NA" AND [P3] =  "NA" 
      ELSE [P2] <> "NA" AND [P3] <> "NA";

This will result in a single test for `P1 = -1` and no `NA`'s anywhere else; and no `P2` and `P3` combinations are used up for the -1 test:

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

However it will get progressively more complex if multiple layers of exclusions are needed. For example let's extend the above with an additional negative test where `P2` is `Null`:

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

## Weighting

The generation mechanism can be forced to prefer certain values. This is done by means of weights. A weight can be any positive integer. If it is not specified explicitly it is assumed to be 1:

    #
    # Let’s focus on primary partitions formatted with NTFS
    #

    Type:           Primary (10), Logical, Single, Span, Stripe, Mirror, RAID-5
    Format method:  quick, slow
    File system:    FAT, FAT32, NTFS (10)

Note:
Weight values have no intuitive meaning. For example, when a parameter is defined like this:

    File system:    FAT, FAT32, NTFS (10)

it does not mean that NTFS will appear in the output ten times more often than FAT or FAT32. Moreover, you cannot be sure that the weights you specified will be honored at all.

The reason for this is that we deal with two contradictory requirements:
 1. To cover all combinations in the smallest number of test cases.  
 2. To choose values proportionally to their weights. 

(1) will always take precedence over (2) and weights will only be honored when the choice of a value is not determined by a need of satisfying (1). More specifically, during the test case production, candidate values are evaluated and one that covers most of the still unused combinations is always picked. Sometimes there is a tie among candidate values and really no choice is better than another. In those cases, weights will be used to determine the final choice.

The bottom-line is that you can use weights to attempt to shift the bias towards some values but whether or not to honor that and to what extent is determined by multiple factors, not weights alone.

## Seeding

Seeding makes two scenarios possible:
 1. Allows for specifying important e.g. regression-inducing combinations that should end up in any generated test suite. In this case, PICT will initialize the output with combinations you provide and will build up the rest of the suite on top of them, still making sure all n-order combinations get covered. 
 2. Let's you minimize changes to the output when your model is modified. Provide PICT with results of some previous generation and expect the tool to reuse the old test cases as much as possible. 

Seeding rows must be defined in a separate file (a seeding file). Use new option **/e** to provide the tool with the file location:

    pict.exe model.txt /e:seedrows.txt 

Seeding files have the same format as any PICT output. First line contains parameter names separated by tab characters and each following line contains one seeding row with values also separated by tabs. This format can easily be prepared from scratch (scenario 1) either in Notepad or in Excel and also allows for quick and direct reuse of any prior results (scenario 2).

    Ver      SKU   Lang  Arch
    Win7     Pro   EN    x86 
    Win7           FR    x86 
    Win10    Pro   EN    x64

Any seeding row may be complete i.e. with values specified for all parameters, or partial, like the second seeding row above which does not have a value for the SKU parameter. In this case, the generation process will take care of choosing the best value for SKU.

Important notes:

There are a few rules of matching seeding rows with the current model:
 1. If a seeding file contains a parameter that is not in the current model, the entire column representing the parameter in the seeding file will be discarded. 
 2. If a seeding row contains a value that is not in the current model, the value will be removed from the seeding row. The rest of the row however, if valid, will still be used. As a result of that, a complete row may become partial. 
 3. If a seeding row violates any of the current constraints, it will be skipped entirely. 

PICT will issue warnings if (1) or (2) occurs.

In addition, there are a few things that are normally allowed in PICT models but may lead to ambiguities when seeding is used:
 1. Blank parameter and value names. 
 2. Parameter and value names containing tab characters. 
 3. Values and all their aliases not unique within a parameter. 

When seeding is used, you will be warned if any of the above problems are detected in your model.

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
