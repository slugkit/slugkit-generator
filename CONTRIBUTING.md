> [!CAUTION]
> This is a placeholder, proper contents will be added later.
> I didn't even read the whole document :)

# Contributing to SlugKit Generator

Thank you for your interest in contributing to SlugKit Generator! This document provides guidelines for contributing to this high-performance C++ slug generation library.

## Code of Conduct

We are committed to fostering an open and welcoming environment. Please read and follow our [Code of Conduct](CODE_OF_CONDUCT.md).

## Getting Started

### Prerequisites

- C++20 compatible compiler (GCC 11+, Clang 13+, MSVC 2022+). But only clang was thoroughly tested, use other compilers at your own risk.
- CMake 3.20 or later
- [userver](https://userver.tech) framework
- Git
- Google Test (for running tests)

### Setting Up Development Environment

1. **Fork and clone the repository**
   ```bash
   git clone https://github.com/YOUR_USERNAME/slugkit-generator.git
   cd slugkit-generator
   ```

2. **Install dependencies**
   ```bash
   # TODO: Add dependency installation instructions
   ```

3. **Build the project**
   ```bash
   # TODO: Add build instructions
   ```

4. **Run tests to verify setup**
   ```bash
   # TODO: Add test execution commands
   ```

## Development Workflow

### Branching Strategy

- `main` - Stable release branch
- `develop` - Integration branch for new features
- `feature/descriptive-name` - Feature development branches
- `bugfix/descriptive-name` - Bug fix branches
- `hotfix/descriptive-name` - Critical production fixes

### Making Changes

1. **Create a feature branch**
   ```bash
   git checkout develop
   git pull origin develop
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes**
   - Write clear, focused commits
   - Follow the coding standards below
   - Add or update tests as needed
   - Update documentation if applicable

3. **Test your changes**
   ```bash
   # Run full test suite
   # TODO: Add test commands
   
   # Run benchmarks if performance-related
   # TODO: Add benchmark commands
   ```

4. **Commit and push**
   ```bash
   git add .
   git commit -m "feat: add descriptive commit message"
   git push origin feature/your-feature-name
   ```

5. **Submit a pull request**
   - Target the `develop` branch
   - Provide a clear description of changes
   - Reference any related issues
   - Ensure CI checks pass

## Coding Standards

### C++ Guidelines

We follow modern C++ best practices with emphasis on performance and maintainability:

#### General Principles
- **C++20 standard**: Utilise modern language features appropriately
- **Performance first**: Optimise for speed and memory efficiency
- **Exception safety**: Provide strong exception safety guarantees
- **const correctness**: Use `const` wherever possible
- **RAII**: Manage resources through constructors/destructors

#### Naming Conventions
```cpp
// Classes and types: PascalCase
class PatternParser;
struct GenerationConfig;
using SlugSequence = std::vector<std::string>;

// Functions and variables: snake_case
void parse_pattern(const std::string& pattern);
auto generation_count = 42;

// Constants: kPascalCase
constexpr int kMinPatternLength = 1024;

// Private members: trailing underscore
class Generator {
private:
    std::string pattern_;
    uint64_t seed_;
};
```

#### Code Style
- **Indentation**: 4 spaces (no tabs)
- **Line length**: 120 characters maximum
- **Braces**: Allman style for classes/functions, same-line for control flow
- **Headers**: `#pragma once` preferred over include guards
- **Documentation**: Doxygen-style comments for public APIs

```cpp
/// @brief Generates slugs based on parsed patterns
/// @param pattern The pattern template to use
/// @param seed Deterministic seed value
/// @param count Number of slugs to generate
/// @return Vector of generated slug strings
std::vector<std::string> generate_slugs(
    const ParsedPattern& pattern,
    uint64_t seed,
    size_t count);
```

#### Performance Guidelines
- **Avoid unnecessary allocations**: Prefer stack allocation and move semantics
- **Use string views**: `std::string_view` for read-only string parameters
- **Reserve container capacity**: Pre-allocate when final size is known
- **Inline hot paths**: Mark frequently called functions `inline`
- **Profile before optimising**: Use benchmarks to validate performance claims

### Error Handling
- Use exceptions for exceptional conditions
- Provide strong exception safety guarantees
- Document exception specifications in headers

```cpp
class PatternParseError : public std::runtime_error {
public:
    explicit PatternParseError(const std::string& message)
        : std::runtime_error("Pattern parse error: " + message) {}
};
```

## Testing Guidelines

### Test Organisation
- Unit tests in `tests/unit/`
- Integration tests in `tests/integration/`
- Benchmark tests in `benchmarks/`
- Test data in `tests/data/`

### Writing Tests
```cpp
#include <gtest/gtest.h>
#include <slugkit/pattern_parser.hpp>

class PatternParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        // TODO: Setup test fixtures
    }
};

TEST_F(PatternParserTest, ParsesBasicPattern) {
    // TODO: Add test implementation
}
```

### Test Coverage
- Aim for >90% line coverage on core functionality
- Test edge cases and error conditions
- Include performance regression tests
- Validate memory usage patterns

## Documentation

### Code Documentation
- Document all public APIs with Doxygen comments
- Include usage examples for complex functions
- Document performance characteristics
- Explain algorithmic choices for non-obvious code

### User Documentation
- Update README.md for new features
- Add examples to the example application
- Update pattern grammar documentation
- Document breaking changes in CHANGELOG.md

## Performance Considerations

### Benchmarking
- Add benchmarks for new features affecting performance
- Compare against baseline measurements
- Document performance characteristics
- Profile memory usage for bulk operations

### Optimisation Guidelines
- Measure before optimising
- Focus on algorithmic improvements first
- Consider cache-friendly data structures
- Minimise dynamic memory allocation in hot paths

## Submitting Changes

### Pull Request Guidelines
1. **Clear title**: Use conventional commit format
   - `feat: add roman numeral generator`
   - `fix: handle empty dictionary gracefully`
   - `perf: optimise pattern parsing performance`
   - `docs: update pattern grammar specification`

2. **Comprehensive description**:
   - What changes were made and why
   - Performance impact (if applicable)
   - Breaking changes (if any)
   - Related issues or discussions

3. **Testing checklist**:
   - [ ] All existing tests pass
   - [ ] New tests added for new functionality
   - [ ] Benchmarks updated if performance-related
   - [ ] Documentation updated
   - [ ] No performance regressions

### Review Process
- All changes require at least one review
- Performance-critical changes require benchmark validation
- Breaking changes require discussion in issues first
- Maintainers will provide constructive feedback

## Getting Help

### Communication Channels
- **GitHub Issues**: Bug reports and feature requests
- **GitHub Discussions**: General questions and design discussions
- **Pull Request Reviews**: Code-specific feedback

### Resources
- [userver Documentation](https://userver.tech)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)

## Recognition

Contributors will be recognised in:
- CONTRIBUTORS.md file
- Release notes for significant contributions
- GitHub contributor statistics

We appreciate all forms of contribution, from code and documentation to bug reports and feature suggestions!

## Licence Agreement

By contributing to SlugKit Generator, you agree that your contributions will be licensed under the same licence as the project.
