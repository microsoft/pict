---
layout: default
---

[(back)](./)

The basic measure of efficiency of a pairwise test generation tool is the number of tests a tool generates given some size of input. For example, when the input has four parameters with 3 values each, denoted 3<sup>4</sup> in the table below, tools create between 9 and 11 test cases. All of the test suites meet the pairwise testing criterion of covering each pair of values in at least one test case however some tools can pack all these combinations them into fewer tests. This may not matter for small problems, test suites for large test domains can exhibit bigger differences.

Test generation efficiency is one aspect of a tool that a user will want to consider when choosing a tool to use, but numbers are easier to compare than less tangile aspects like "usability", so a standard set of benchmarks emerged over the years. The table below summarizes efficiencies for several tools that happened to publish their numbers.  

| # | Model | 3<sup>4</sup> | 3<sup>13</sup> | 4<sup>15</sup> 3<sup>17</sup> 2<sup>29</sup> | 4<sup>1</sup> 3<sup>39</sup> 2<sup>35</sup> | 2<sup>100</sup> | 10<sup>20</sup> |
| :---- | :----: | ----: | ----: | ----: | ----: | ----: | ----: | 
| 1 | AETG  | 9 | 15 | 41 | 28 | 10 | 180 |
| 2 | IPO  | 9 | 17 | 34 | 26 | 15 | 212 |
| 3 | TConfig  | 9 | 15 | 40 | 30 | 14 | 231 |
| 4 | CTS  | 9 | 15 | 39 | 29 | 10 | 210 |
| 5 | Jenny  | 11 | 18 | 38 | 28 | 16 | 193 |
| 6 | TestCover  | 9 | 15 | 29 | 21 | 10 | 181 |
| 7 | DDA  | ? | 18 | 35 | 27 | 15 | 201 |
| 8 | AllPairs [McDowell]  | 9 | 17 | 34 | 26 | 14 | 197 |
| 9 | PICT | 9 | 18 | 37 | 27 | 15 | 210 |
| 10 | EXACT | 9 | 15 | ? | 21 | 10 | ? |
| 11 | IPO-s  | 9 | 17 | 32 | 23 | 10 | 220 |
| 12 | ecFeed  | 10 | 19 | 37 | 28 | 16 | 203 |
| 13 | JCUnit  | 10 | 23 | 49 | 33 | 18 | 245 |
| 14 | CoverTable  | 9 | 17 | 34 | 26 | 12 | 195 |

