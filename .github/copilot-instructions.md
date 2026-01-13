# PICT Repository Development Guidelines

Comprehensive instructions for developing in the PICT (Pairwise Independent Combinatorial Testing) repository

## Project Overview

PICT (Pairwise Independent Combinatorial Testing) is a Microsoft open-source command-line tool and C++ library for generating test cases using combinatorial testing methodology. The tool generates compact sets of parameter value combinations that provide comprehensive pairwise (or higher-order) coverage for testing complex systems with multiple interacting parameters.

**Key Capabilities:**
- Generates combinatorial test suites (pairwise, 3-way, n-way coverage)
- Supports complex constraint expressions to model real-world limitations  
- Provides both command-line interface and programmatic C++ API
- Cross-platform support (Windows, Linux, macOS)
- Handles large parameter spaces efficiently using advanced algorithms

## Architecture & Components

### Core Components
- **api/**: Core combinatorial generation engine (C++ library)
  - `pictapi.h/cpp`: Main public API interface
  - `model.cpp`: Model representation and management
  - `generator.h`: Core generation algorithms
  - `parameter.cpp`: Parameter and value management
  - `combination.cpp`: Combination handling logic

- **cli/**: Command-line interface (pict.exe)
  - `pict.cpp`: Main entry point and CLI logic
  - `ccommon.h/cpp`: Common CLI utilities and constraint parsing
  - `mparser.cpp`: Model file parsing

- **clidll/**: Windows DLL wrapper for CLI functionality
- **test/**: Comprehensive Perl-based test suite with baseline comparisons

### Build Systems
- **Windows**: Visual Studio (.sln) with MSBuild
- **Cross-platform**: CMake (minimum version 3.13)
- **Unix/Linux**: Traditional Makefile
- **Container**: Containerfile for containerized deployment

## Coding Standards & Conventions

### Language & Standards
- **C++17** standard required (set via `CMAKE_CXX_STANDARD`)
- Cross-platform compatibility is mandatory
- Use RAII patterns for resource management
- Prefer standard library containers and algorithms

### API Design Patterns
- Public API functions prefixed with `PICT_` (e.g., `PictCreateModel`, `PictGenerate`)
- Return handles/pointers for opaque types (e.g., `PICT_TASK`, `PICT_MODEL`)
- Use `IN`/`OUT` parameter annotations for clarity
- Provide both C++ and C-compatible interfaces

### Error Handling
- Use exceptions internally, convert to error codes at API boundaries
- Validate all inputs and provide meaningful error messages
- Handle edge cases gracefully (empty models, invalid constraints, etc.)
- Print errors and warnings to stderr, results to stdout

### Memory Management
- Use smart pointers and RAII where possible
- Provide explicit cleanup functions for API consumers
- Avoid memory leaks in long-running scenarios
- Handle large parameter spaces efficiently

## Common Development Patterns

### Model Processing Flow
1. **Parse** model file or programmatically build model
2. **Validate** parameters, constraints, and sub-models
3. **Generate** combinations using core algorithms
4. **Filter** results based on constraints
5. **Output** formatted results

### Constraint Handling
- Support complex boolean expressions with AND, OR, NOT
- Handle relational operators: `=`, `<>`, `>`, `>=`, `<`, `<=`, `LIKE`
- Process `IF...THEN...ELSE` conditional constraints
- Support parameter aliasing and value sets (`IN {values}`)

### Testing Patterns
- Every feature must have corresponding test cases in `test/`
- Use baseline comparison for regression testing
- Test cross-platform behavior on Windows, Linux, macOS
- Include performance tests for large parameter spaces
- Test Unicode/encoding scenarios with non-ASCII characters

## Common Issues & Solutions

### Performance Optimization
- **Large models**: Use sub-models to partition parameter spaces
- **Many constraints**: Optimize constraint evaluation order
- **Memory usage**: Stream results when possible, avoid keeping all combinations in memory
- **Generation time**: Consider using randomization (`/r` option) for different heuristic paths

### Encoding & Internationalization
- **Unicode support**: Properly handle UTF-8, UTF-16 input files
- **Cross-platform paths**: Use appropriate path separators and encoding
- **Non-ASCII parameters**: Test with Chinese, Japanese, and other character sets
- **File I/O**: Handle BOM markers and different text encodings

### API Usage Patterns
```cpp
// Typical API usage pattern
PICT_TASK task = PictCreateTask();
PICT_MODEL model = PictCreateModel(randomSeed);
PictSetRootModel(task, model);

// Add parameters
PictAddParameter(model, valueCount);
// ... add more parameters

// Generate combinations
PictGenerate(task);
PictResetResultFetching(task);

// Fetch results
PICT_RESULT_ROW row = PictAllocateResultBuffer(task);
while (PictGetNextResultRow(task, row)) {
    // Process row
}

// Cleanup
PictFreeResultBuffer(row);
PictDeleteModel(model);  
PictDeleteTask(task);
```

### Constraint Syntax
- Use square brackets around parameter names: `[ParamName]`
- Quote string values: `[Param] = "value"`
- Support wildcards in LIKE: `[Param] LIKE "prefix*"`
- Chain conditions with AND, OR: `[A] = 1 AND [B] <> 2`

## Build & Development Workflow

### Building
```bash
# Cross-platform CMake build
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
cmake --build build

# Traditional make (Linux/macOS)
make

# Windows MSBuild
msbuild pict.sln /p:Configuration=Release
```

### Testing
```bash
# Run full test suite
make test  # or ctest in CMake build

# Update test baselines (when output format changes)
_update_baselines.cmd  # Windows
```

### Code Quality
- Enable warnings as errors (`-Werror`, `/WX`)
- Use static analysis tools (CodeQL in CI)
- Run tests on all supported platforms before merging
- Maintain backward compatibility for API changes

## Development Guidelines

### When Adding Features
1. **Design**: Consider both CLI and API usage patterns
2. **Implement**: Follow existing code patterns and conventions
3. **Test**: Add comprehensive test cases with edge cases
4. **Document**: Update both code comments and user documentation
5. **Validate**: Test on multiple platforms and with various inputs

### When Fixing Bugs  
1. **Reproduce**: Create minimal test case demonstrating the issue
2. **Root cause**: Understand why the issue occurs
3. **Fix**: Make minimal, targeted changes
4. **Verify**: Ensure fix doesn't break existing functionality
5. **Test**: Add regression test to prevent recurrence

### Performance Considerations
- Profile before optimizing - measure actual bottlenecks
- Consider algorithmic improvements over micro-optimizations  
- Test with realistic large models from user scenarios
- Monitor memory usage patterns for long-running processes

### Backward Compatibility
- Maintain API stability - deprecate rather than remove
- Support existing model file formats
- Preserve command-line interface behavior
- Document any breaking changes clearly

## File Organization & Naming

### Code Files
- Header files: `.h` extension
- Implementation: `.cpp` extension  
- Platform-specific: Use `#ifdef` guards rather than separate files
- Test files: Descriptive names in `test/` subdirectories

### Test Organization
- `test/modl/`: Model file tests
- `test/cons/`: Constraint tests  
- `test/func/`: Functional tests
- `test/real/`: Real-world scenario tests
- `test/errors/`: Error condition tests

This repository represents a mature, stable tool used by testing teams worldwide. Changes should prioritize correctness, performance, and backward compatibility while maintaining the high quality standards established by the existing codebase.
