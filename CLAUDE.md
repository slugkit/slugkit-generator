# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**SlugKit Generator** is a high-performance C++ library for generating aesthetically pleasing, deterministic human-readable identifiers using configurable patterns. It powers the [SlugKit service](https://dev.slugkit.dev) and provides:

- Pattern-based generation with dictionary selectors, number generators, and special characters
- Dictionary integration with language and tag-based filtering
- Deterministic, seed-based generation for reproducible results
- High performance (25ns - 3µs generation time depending on pattern complexity)
- Minimal dependencies (only requires `userver::core`)

## Commands

### Build Commands

The project uses CMake with conditional compilation for different components:

```bash
# Basic build (requires CMake 3.20+ and C++20 compiler)
mkdir build && cd build
cmake .. -DSLUGKIT_GENERATOR_BUILD_TESTS=ON -DSLUGKIT_GENERATOR_BUILD_BENCHMARKS=ON -DSLUGKIT_GENERATOR_BUILD_EXAMPLES=ON
make

# Build specific components
cmake .. -DSLUGKIT_GENERATOR_BUILD_TESTS=ON     # Enable tests
cmake .. -DSLUGKIT_GENERATOR_BUILD_BENCHMARKS=ON # Enable benchmarks  
cmake .. -DSLUGKIT_GENERATOR_BUILD_EXAMPLES=ON   # Enable examples
```

### Testing Commands

```bash
# Build and run all tests
make && make test

# Run specific test executable directly
./test_slugkit-generator

# Individual test components (located in slugkit/tests/):
# - numerics_test.cpp - Numeric utility tests
# - roman_test.cpp - Roman numeral generation tests
# - pattern_test.cpp - Pattern parsing and validation tests
# - permutation_test.cpp - Permutation generation tests
# - dictionary_test.cpp - Dictionary loading and filtering tests
# - generator_test.cpp - Main generation engine tests
# - emoji_generator_test.cpp - Emoji generation tests
```

### Benchmarking Commands

```bash
# Run benchmarks (requires google/benchmark)
./slugkit-benchmark

# Individual benchmark components (located in slugkit/benchmarks/):
# - permute_benchmark.cpp - Permutation algorithm benchmarks
# - generate_slugs_benchmarks.cpp - End-to-end generation benchmarks
# - filter_dictionary_benchmarks.cpp - Dictionary filtering performance
# - index_benchmarks.cpp - Dictionary index performance
```

### Examples

```bash
# Run YAML dictionary example
./yaml-dict -f dictionary.yaml -p '{adjective}-{noun}-{number:4R}' -c 1000 -s "test-seed"

# Example patterns:
# {adjective}-{noun}                    # Simple adjective-noun combination
# {adjective@en:+formal-nsfw>3}-{noun}  # Complex with language, tags, length constraints
# {number:5d}                           # 5-digit decimal number
# {special:3-5}                         # 3 to 5 special characters
# {emoji}                               # Single random emoji
# {emoji:+face}                         # Face emoji with tag filtering
# {emoji:+face count=3}                 # 3 face emoji (can repeat)
# {emoji:+face count=2 unique=true}     # 2 unique face emoji
# {emoji:+face count=1-4}               # Variable count (1-4) face emoji
```

## Architecture

### Core Libraries

The library is structured into several key components:

#### Generator Engine (`slugkit/src/slugkit/generator/`)
- **`generator.cpp`** - Main `Generator` class providing the public API
- **`pattern.cpp`** - Pattern parsing and validation (`Pattern` class)
- **`pattern_generator.cpp`** - Core generation logic and template substitution
- **`dictionary.cpp`** - Dictionary management and filtering (`DictionarySet`, `Dictionary`)
- **`placeholders.cpp`** - Placeholder implementations (`Selector`, `NumberGen`, `SpecialCharGen`, `EmojiGen`)
- **`permutations.cpp`** - Permutation algorithms for deterministic generation
- **`detail/pattern_parser.cpp`** - EBNF-based pattern parsing implementation
- **`detail/indexes.cpp`** - Dictionary indexing and caching for performance

#### Utility Libraries (`slugkit/src/slugkit/utils/`)
- **`numeric.cpp`** - Number generation utilities and base conversions
- **`roman.cpp`** - Roman numeral generation
- **`text.cpp`** - Text processing and string utilities
- **`primes.cpp`** - Prime number utilities for hash generation

#### Headers Structure (`slugkit/include/slugkit/`)
- **`generator/`** - Main API headers
  - `generator.hpp` - Main `Generator` class interface
  - `pattern.hpp` - `Pattern` class and parsing functions
  - `dictionary.hpp` - Dictionary types and filtering
  - `structured_loader.hpp` - YAML/JSON dictionary loading
  - `placeholders.hpp` - Placeholder type definitions
- **`utils/`** - Utility headers
  - `numeric.hpp` - Number generation utilities
  - `roman.hpp` - Roman numeral conversion
  - `text.hpp` - Text processing utilities
- **`generator/io/`** - Optional serialisation support
  - `types_io.hpp` - JSON/YAML serialisation for core types
  - `pattern_io.hpp` - Pattern serialisation utilities

### Pattern Language

The library implements a powerful EBNF-based pattern grammar supporting:

- **Dictionary selectors**: `{adjective}`, `{noun@en}`, `{verb:+formal-slang}`
- **Number generators**: `{number:4x}`, `{num:6d}`, `{number:3R}`
- **Special characters**: `{special:2}`, `{spec:1-4}`
- **Emoji generation**: `{emoji}`, `{emoji:+face}`, `{emoji:+animals-nsfw count=3 unique=true}`
- **Length constraints**: `{adjective:<8}`, `{noun:>=5}`
- **Tag filtering**: `{word:+formal-slang-nsfw}` (include/exclude tags)

### Key Classes

#### `Generator` (`generator.hpp`)
Main generation engine providing:
- Single and batch generation methods
- Capacity calculation for patterns
- Random seed generation
- Thread-safe operation

#### `Pattern` (`pattern.hpp`) 
Immutable pattern representation with:
- EBNF parsing of pattern strings
- Placeholder extraction and validation
- Complexity calculation
- Hash generation for caching

#### `Dictionary` (`dictionary.hpp`)
Dictionary management supporting:
- Multi-language word storage
- Tag-based filtering
- Length constraints
- Indexed lookups for performance

### Dependencies

- **Core**: `userver::core` (for text utilities and strong typedefs)
- **Optional serialisation**: `userver::universal` (for JSON/YAML support)
- **Testing**: `userver::utest` (Google Test framework)
- **Benchmarking**: `google/benchmark`

### Data Files

- **`data/emoji.yaml`** - Embedded emoji dictionary with categorisation and tags (~1200 emoji)
- **`slugkit/examples/yaml-dict/dictionary.yaml`** - Example word dictionary for testing

### Emoji Generation Features

The library includes comprehensive emoji support through the `EmojiGen` placeholder:

#### Emoji Pattern Syntax
- **`{emoji}`** - Single random emoji from embedded dictionary
- **`{emoji:+tag}`** - Filtered by include tags (e.g., `+face`, `+animals`, `+food`)
- **`{emoji:-tag}`** - Filtered by exclude tags (e.g., `-nsfw`, `-violence`)
- **`{emoji:count=N}`** - Generate exactly N emoji (1-6, can repeat)
- **`{emoji:count=N-M}`** - Variable count between N and M emoji
- **`{emoji:unique=true}`** - Ensure all generated emoji are unique (requires `count>1`)
- **`{emoji:+face count=3 unique=true}`** - Complex: 3 unique face emoji

#### Implementation Details
- **`EmojiSubstitutionGenerator`** - Core generation engine using embedded dictionary
- **Tag-based filtering** - Extensive categorisation system (face, animals, food, symbols, etc.)
- **Deterministic generation** - Same seed produces same emoji sequence
- **Performance optimised** - Pre-computed capacity calculations and caching
- **Embedded dictionary** - ~1200 emoji compiled into binary (no external files needed)
- **Unicode support** - Handles complex emoji including skin tones, flags, and multi-codepoint sequences

#### Capacity and Constraints
- **Maximum emoji count**: 6 per placeholder (defined by `kMaxEmojiCount`)
- **Unique mode capacity**: Calculated using permutations for non-repeating selection
- **Tag filtering impact**: Reduces available emoji pool, affecting total capacity
- **Variable count ranges**: Cumulative capacity calculation across count range

### Performance Characteristics

- **Single generation**: 3-20μs (depending on pattern complexity)
- **Bulk operations**: Optimised per-slug performance through batching
- **Memory usage**: Dictionary caching and indexed lookups minimise allocations
- **Deterministic**: Same seed + sequence always produces same results

### Development Workflow

1. **Core changes** typically require updating both headers in `include/` and implementations in `src/`
2. **New placeholder types** need updates to `placeholders.hpp/.cpp` and pattern parser
3. **Performance changes** should include corresponding benchmark updates
4. **Pattern grammar changes** require updates to EBNF documentation and parser
5. **Dictionary changes** may require updates to both loading and filtering logic

The library is designed as a dependency for larger CMake projects and integrates through `add_subdirectory()`.