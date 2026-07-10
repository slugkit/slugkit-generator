# SlugKit Generator

A high-performance C++ library for generating aesthetically pleasing, deterministic slugs with comprehensive pattern support. This is the core generation engine that powers the [SlugKit service](https://dev.slugkit.dev).

## Features

- **Pattern-Based Generation**: Dictionary selectors, number/special/emoji generators, plus alternation and grouping for choice and correlated selection
- **Dictionary Integration**: Load and filter word dictionaries with language and tag-based constraints, including hidden opt-in tags
- **Deterministic Output**: Seed-based generation ensures reproducible results
- **High Performance**: Optimised C++ implementation with *about*-millisecond generation times (25ns - 3µs, depending on pattern complexity)
- **Minimal Dependencies**: Builds against `userver::core`, or fully **userver-free** as a standalone library (`-DSLUGKIT_USE_USERVER=OFF`) for embedding and mobile
- **C ABI & Mobile Bindings**: A stable C ABI (`slugkit/c/slugkit_c.h`) with Swift, Kotlin, Dart, and Flutter bindings
- **Optional Serialisation**: JSON/YAML support available as separate headers

## Quick Start

### Installation

> [!NOTE]
> Instructions how to plug this library into a project are required

```bash
# TODO: Add installation instructions
```

### Basic Usage

```cpp
#include <slugkit/generator/generator.hpp>
// for loading dictionaries from JSON/YAML files
#include <slugkit/generator/structured_loader.hpp>

```

<details>

<summary>Rest of includes and boring main boilerplate</summary>

```cpp
#include <userver/engine/run_standalone.hpp>
#include <userver/formats/yaml.hpp>

#include <boost/program_options.hpp>

#include <fstream>
#include <iostream>

#include <fmt/format.h>

int main(int argc, char* argv[]) {
    namespace po = boost::program_options;
    po::options_description desc("YAML Dictionary Generator");
    // clang-format off
    desc.add_options()
        ("help,h", "produce help message")
        ("file,f", po::value<std::string>()->required(), "file to read")
        ("pattern,p", po::value<std::string>()->required(), "pattern to use")
        ("count,c", po::value<std::size_t>()->default_value(1), "number of slugs to generate")
        ("sequence,n", po::value<std::size_t>()->default_value(0), "sequence number")
        ("seed,s", po::value<std::string>(), "seed for the generator. If not provided, a random seed will be used")
    ;
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }
    po::notify(vm);

    auto file_name = vm["file"].as<std::string>();
    std::ifstream file(file_name);
    if (!file.is_open()) {
        throw std::runtime_error(fmt::format("Failed to open file: {}", file_name));
    }

    auto pattern = vm["pattern"].as<std::string>();
    auto sequence = vm["sequence"].as<std::size_t>();
    std::string seed;
    if (vm.count("seed")) {
        seed = vm["seed"].as<std::string>();
    } else {
        seed = generator.RandomSeed();
    }
    auto count = vm["count"].as<std::size_t>();

```

</details>

**Generating slugs**

```cpp
    // Read yaml from file stream
    auto yaml = userver::formats::yaml::FromStream(file);
    // Parse dictionaries from the file
    auto dictionary_set = yaml.As<slugkit::generator::DictionarySet>();
    // Initialize the generator
    slugkit::generator::Generator generator(std::move(dictionary_set));

    userver::engine::RunStandalone([&] {
        if (count == 1) {
            std::cout << generator(pattern, seed, sequence) << '\n';
        } else {
            generator(pattern, seed, sequence, count, [](const std::string& slug) { std::cout << slug << '\n'; });
        }
    });
} // end of main
```

**Validating pattern**

```cpp
    // you don't actually need a generator, as the pattern parser check if the syntax is fine
    // it won't check for dictionary or tag presense
    slugkit::generator::Pattern parsed_pattern{pattern};
    std::cout << "Pattern complexity: " << parsed_pattern.Complexity() << '\n';
```

## Pattern Language

SlugKit uses a powerful pattern language that supports multiple element types:

### Dictionary Selectors
- `{adjective}` - Basic word selection
- `{noun@en}` - Language-specific selection  
- `{verb:+formal-slang}` - Tag-based filtering
- `{adjective:<8}` - Length constraints

### Number Generators
- `{number:4x}` - 4-digit lowercase hexadecimal
- `{num:6d}` - 6-digit decimal
- `{number:3R}` - 3-character Roman uppercase numerals

### Special Character Generators
- `{special:2}` - Exactly 2 special characters
- `{spec:1-4}` - 1 to 4 special characters

### Emoji Generators
- `{emoji}` - Single random emoji from built-in collection (~1200 emoji)
- `{emoji:+face}` - Face emoji only (tag filtering)
- `{emoji:+animals-nsfw}` - Animal emoji, excluding NSFW content
- `{emoji:count=3}` - Exactly 3 emoji (repetition allowed)
- `{emoji:count=2-4}` - Variable count between 2-4 emoji
- `{emoji:+face count=2 unique=true}` - 2 unique face emoji

### Alternation

Use `|` to choose one of several alternatives. Capacity is the **sum** of the branches, and generation stays deterministic and collision-free across the whole set.

- `{adjective}|{noun}` - an adjective **or** a noun
- `{noun}|{verb}|{number:3d}` - one of three
- `x-{adjective}|{noun}-y` - surrounding text applies to the whole alternation

Branches whose outputs would coincide are a pattern error (e.g. `{noun}|{noun}`, or `{adverb}|{adverb:+pos}` where one branch is a superset of the other). Byte-identical branches collapse rather than double-count.

### Groups & Correlated Choices

Parentheses group a run of placeholders and literal text into a single unit. Alternating groups locks correlated choices together, so aligned placeholders vary in step rather than independently:

- `({name:+given+male} {name:+surname+male})|({name:+given+female} {name:+surname+female})` - a consistently-gendered full name
- `(foo)|(bar)` - pure-text branches
- `({adverb})|()` - an **optional** element: an adverb *or* nothing

The empty branch `()` is an explicit "or nothing" option worth one unit of capacity. A lone group is transparent — `({noun})` is exactly `{noun}` — and a standalone `()` is rejected (it is only meaningful alongside other branches). Groups are flat: no nested groups or alternations. To use `(`, `)` or `|` as literal text, escape them (`\(`, `\|`).

### Opt-in Tags

A tag may be flagged **opt-in** in the dictionary (e.g. `nsfw`). Words carrying an opt-in tag are hidden by default; they appear only when the tag is requested explicitly — `{noun:+nsfw}` — or enabled on the generator as a runtime usage flag:

```cpp
generator.EnableOptIn("nsfw");   // un-hides nsfw words for this generator; ClearOptIns() restores the default
```

Enabling a tag un-hides its words without restricting output to them (unlike `+tag`). This is a usage flag, not part of the pattern grammar.

### Pattern Grammar (EBNF)

```ebnf
pattern           := ARBITRARY, { element, ARBITRARY }, [ global_settings ];
element           := alternation | group | placeholder;
alternation       := group_or_ph, ('|', group_or_ph)+;   {* >= 2 distinct branches after collapse *}
group_or_ph       := group | placeholder;
group             := '(', group_body, ')';               {* '()' is valid only as an alternation branch *}
group_body        := ARBITRARY, { placeholder, ARBITRARY };  {* flat: no nested groups or alternation *}
placeholder       := '{', (selector | number_gen | special_char_gen | emoji_gen), '}';
selector          := kind ['@' lang], [':', [tags], [length_constraint], [options]];
global_settings   := '[' ['@' lang], [tags], [length_constraint], [options] ']';
number_gen        := 'number', ':', length, [(',', number_base) | number_base_short ];
special_char_gen  := 'special', [':', number, ['-', length]];
emoji_gen         := 'emoji', [':', [tags], [options]]
kind              := identifier;
lang              := identifier;
tags              := (include_tag | exclude_tag)*;
include_tag       := '+', tag;
exclude_tag       := '-', tag;
length_constraint := comparison_op, length;
comparison_op     := eq | ne | lt | le | gt | ge;
options           := option (' ' option)*;
option            := identifier '=' option_value;
tag               := (ALNUM | '_')+;
identifier        := (ALPHA | '_'), (ALNUM | '_')*;
option_value      := tag | number;
eq                := '==';
ne                := '!=';
lt                := '<';
le                := '<=';
gt                := '>';
ge                := '>=';
length            := number;
number_base       := 'dec' | 'hex' | 'HEX' | 'roman' | 'ROMAN';
number_base_short := 'd' | 'x' | 'X' | 'r' | 'R';

number            := '0' | NON_ZERO_DIGIT, { DIGIT };
NON_ZERO_DIGIT    := '1'..'9';
DIGIT             := '0'..'9';

ALPHA             := 'a'..'z' | 'A'..'Z';
ALNUM             := ALPHA | DIGIT;

ARBITRARY         := { CHAR_NO_RESERVED | ESCAPED_CHAR };
CHAR_NO_RESERVED  := ? any character except '{', '}', '(', ')', '|', '\' ?;

ESCAPED_CHAR      := escape_symbol, ('{' | '}' | '(' | ')' | '|' | escape_symbol);

{* Reserved characters (braces, group parens, the alternation bar) and the escape
   symbol itself must be escaped to appear as literal text *}
escape_symbol     := '\';
```

## API Reference

### Core Classes

#### `Pattern`

Parses pattern strings into executable generation templates. [^1]

```cpp
#include <slugkit/generator/pattern.hpp>

#include <iostream>
#include <memory>

int main(int argc, char* argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " 'pattern'"
    }
    using namespace slugkit::generator;
    Pattern pattern{argv[1]};
    std::cout << "Pattern complexity: " << pattern.Complexity() << "\n";
}
```

#### `Generator` 
Main generation engine for producing slugs from parsed patterns. [^1]

```cpp
#include <slugkit/generator/generator.hpp>

#include <string_view>
#include <iostream>

int main(int argc, char* argv)
{
    if (argc < 2) {
        std::cerr << << "Usage: " << argv[0] << " 'pattern' <optional count>"
    }
    using namespace slugkit::generator;
    // init the generator with dictionaries
    Generator generator{/* dictionaries */};
    
    std::string_view pattern{argv[1]};
    auto seed = generator.RandomSeed();
    if (argc > 2) {
        auto count = std::stoi(argv[2]);
        // Batch generation
        generator(
            pattern,
            seed,
            /* sequence number */0,
            count,
            [](auto slug) {
                std::cout << slug << "\n";
            }
        );
    } else {
        // Generate one
        std::cout << generator(pattern, seed, /* sequence number */ 0)
    }
}
```

#### `Dictionary`

Handles storing and filtering of word dictionaries.

> [!TIP]
> You can use the [`structured_loader.hpp`](slugkit/include/slugkit/generator/structured_loader.hpp) 
> header to parse a DictionarySet from YAML or JSON.

```cpp
// TODO: Add loading dictionaries examples
```

#### `PatternSettings`

Datatype used for storing selected dictionary sizes to avoid sequence skewing when a dictionary size changes.

[^1]: This is example code, to run such a main you'll need to wrap the code in `userver::engine::RunStandalone` call.

## Optional Serialisation Support

For applications requiring JSON or YAML serialisation:

```cpp

#include <slugkit/generator/serialization.hpp>  // Template serialisers/parsers
#include <userver/formats/json.hpp> // JSON support

```

These headers are optional and not included by default to minimise dependencies.

### Dictionary Support

The library supports loading dictionaries from text-based structured files with the following structure:

<details>
<summary>JSON and YAML formats for dictionaries</summary>

```json
{
    "noun" : {
        "lang": "en",
        "words": {
            "slug": [
                    "action",
                    "artifact",
                    "device",
                    "event",
                    "object"
                ]            
            // ...
        }
    }
}
```

```yaml
noun:
    lang: en
    words:
      slug:
        - action
        - artifact
        - device
        - event
        - object
        # ...
```

</details>

> [!IMPORTANT]
> **Note**: Dictionaries are not included with the library. See the example application for sample dictionaries.


## Command-line tool

The `slugkit` CLI generates slugs from **compiled binary dictionaries** and is **fully userver-free** — build the library standalone and it depends only on `fmt`, `utf8proc`, and the header-only [CLI11](https://github.com/CLIUtils/CLI11) (fetched automatically):

```bash
cmake -S slugkit -B build -DSLUGKIT_USE_USERVER=OFF -DSLUGKIT_GENERATOR_BUILD_CLI=ON
cmake --build build --target slugkit

# Compile a source YAML dictionary to the binary format first (Python tool, see slugkit/py/tools),
# then generate:
./build/cli/slugkit -b nouns.noun.bin -b adjectives.adjective.bin \
    -p '{adjective}-{noun}-{number:4R}' -c 1000
./build/cli/slugkit -b colours.colour.bin -p '{colour@fr}' -c 5 --enable-opt-in nsfw
```

Run `slugkit --help` for all options (`--bin`, `--pattern`, `--count`, `--sequence`, `--seed`, `--enable-opt-in`, `--quiet`).

## YAML Example (userver build)

The repository also includes a [YAML example](slugkit/examples/yaml-dict/main.cpp) that loads a dictionary straight from YAML. Because YAML parsing goes through userver's formats, it only builds in the userver build (`SLUGKIT_USE_USERVER=ON`):

```bash
./yaml-dict -f dictionary.yaml -p '{Adjective} {Noun} {number:4R}' -c 1000
./yaml-dict -f dictionary.yaml -p '{emoji:+face}-{adjective}-{noun}' -c 100 -s "emoji-test"
```

## Performance

- **Single slug generation**: 25ns - 3μs depending on pattern complexity
- **Bulk operations**: Improved per-slug performance at scale
- **Memory efficient**: Optimised dictionary loading and caching
- **Advanced permutation algorithms**: Feistel networks for power-of-2 spaces, LCG for arbitrary ranges

> **Performance Disclaimer**: These performance figures are measured in uncongested environments with dedicated CPU resources. Performance may be significantly worse in CPU-constrained or high-contention scenarios.

### Permutation Engine

SlugKit uses sophisticated permutation algorithms to ensure deterministic, collision-free generation:

- **Feistel Networks**: For power-of-2 dictionary sizes, providing cryptographically strong permutations
- **Linear Congruential Generators (LCG)**: For arbitrary dictionary sizes with guaranteed full-period coverage
- **FNV-1a Hashing**: Fast, collision-resistant seed hashing for deterministic behaviour
- **Unique/Non-unique Permutations**: Efficient algorithms for both repeating and non-repeating element selection
- **Mathematical Precision**: P(N,K) calculations for exact capacity planning

The permutation system supports:
- Arbitrary sequence lengths up to 2^64
- Both unique permutations (P(N,K) = N!/(N-K)!) and repetition-allowed permutations (N^K)
- Deterministic mapping from sequence numbers to permuted indices
- Optimised algorithms avoiding expensive factorial calculations

* [Benchmark results August 28, 2025 (after implementing caches)](https://dev.slugkit.dev/articles/smitten-mileage-mdx)
* [Benchmark results August 28, 2025 (after adding indexes)](https://dev.slugkit.dev/articles/axiomatic-tutor-mcx)
* [Benchmark results August 27, 2025](https://dev.slugkit.dev/articles/governing-faculty-mli)

## Building

### Requirements

- C++20 compatible compiler
- CMake 3.20+
- [userver](https://userver.tech) framework (core components only) — **optional**; omit it with `-DSLUGKIT_USE_USERVER=OFF` for a standalone, dependency-light build (uses `utf8proc` + `fmt` instead)

### Build Instructions

> [!NOTE]
> The library is consumed via `add_subdirectory`. It builds against userver by default;
> set `-DSLUGKIT_USE_USERVER=OFF` for a userver-free build — this is what the C ABI and
> the Swift/Kotlin/Dart/Flutter bindings are built on.

```bash
mkdir build
cd build
cmake ..
make
```

### Running Tests

When plugged into build sytem, it adds a CTest target which is run by `make test`

```bash
cd build
make && make test
```

## Dependencies

- **Core library (default)**: `userver::core` (text utilities only)
- **Standalone build** (`SLUGKIT_USE_USERVER=OFF`): no userver — uses `utf8proc` (Unicode casing) and `fmt`. This is what the C ABI and mobile bindings are built on.
- **Optional serialisation**: JSON/YAML via userver formats
- **Tests**: Google Test framework

## Use Cases

- **User handle generation**: Readable, unique identifiers
- **Product SKUs**: Branded, memorable product codes  
- **API resource identifiers**: SEO-friendly URL components
- **Campaign slugs**: Marketing-friendly identifiers
- **Test data generation**: Deterministic test fixtures

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Planned Features

- [ ] Benchmarking suite
- [x] userver-independent variant (`SLUGKIT_USE_USERVER=OFF`) + C ABI and Swift/Kotlin/Dart/Flutter bindings

## Licence

[Apache License 2.0](LICENSE)

## Related Projects

- [SlugKit Service](https://dev.slugkit.dev) - *Almost* production-ready API service.
- [SlugKit Python SDK](https://github.com/slugkit/slugkit-sdk) - Python SDK for using with SaaS or on-prem variants of SlugKit API service.

## Support

- **Documentation**: [TODO: Add docs link]
- **Issues**: [GitHub Issues](https://github.com/slugkit/slugkit-generator/issues)
- **Discussions**: [GitHub Discussions](https://github.com/slugkit/slugkit-generator/discussions)
