# SlugKit Generator

A high-performance C++ library for generating aesthetically pleasing, deterministic slugs with comprehensive pattern support. This is the core generation engine that powers the [SlugKit service](https://dev.slugkit.dev).

## Features

- **Pattern-Based Generation**: Flexible template system supporting dictionary selectors, number generators, and special characters
- **Dictionary Integration**: Load and filter word dictionaries with language and tag-based constraints
- **Deterministic Output**: Seed-based generation ensures reproducible results
- **High Performance**: Optimised C++ implementation with sub-millisecond generation times
- **Minimal Dependencies**: Core library depends only on `userver::core` for text utilities and strong typedefs
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

### Pattern Grammar (EBNF)

```ebnf
pattern           := ARBITRARY, { placeholder, ARBITRARY }, [ global_settings ];
placeholder       := '{', (selector | number_gen | special_char_gen), '}';
selector          := kind ['@' lang], [':', [tags], [length_constraint], [options]];
global_settings   := '[' ['@' lang], [tags], [length_constraint], [options] ']';
number_gen        := 'number', ':', length, [(',', number_base) | number_base_short ];
special_char_gen  := 'special', [':', number, ['-', length]];
kind              := identifier;
lang              := identifier;
tags              := (include_tag | exclude_tag)*;
include_tag       := '+', tag;
exclude_tag       := '-', tag;
length_constraint := comparison_op, length;
comparison_op     := eq | ne | lt | le | gt | ge;
options           := option (',' option)*;
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

ARBITRARY         := { CHAR_NO_BRACE | ESCAPED_CHAR };
CHAR_NO_BRACE     := ? any character except '{', '}', '\' ?;

ESCAPED_CHAR      := escape_symbol, ('{' | '}' | escape_symbol);

{* We want to escape curly braces and the ecsape symbol itself *}
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


## Example Application

The repository includes a complete [example application](slugkit/examples/yaml-dict/main.cpp) demonstrating:

- Dictionary loading from YAML files
- Bulk slug generation

```bash
./yaml-dict -f dictionary.yaml -p '{Adjective} {Noun} {number:4R}' -c 1000
```

## Performance

- **Single slug generation**: 3-20μs depending on pattern complexity
- **Bulk operations**: Improved per-slug performance at scale
- **Memory efficient**: Optimised dictionary loading and caching

<details>
<summary>Benchmark results</summary>

Benchmarks were run in a arm64 ubuntu docker container on a MacBook M4 Max with 48Gb RAM. Theese are results as of August 27, 2025


### [Seed Hash Computation](slugkit/benchmarks/permute_benchmark.cpp)

| Benchmark                                     | Time  |           CPU | Iterations |     |
| ----                                          | ---   | ---           | ---        | --- |
| FNV1aHash/1 | 0.710 ns | 0.710 ns | 921441116 |  |
| FNV1aHash/2 | 0.964 ns | 0.964 ns | 733653608 |  |
| FNV1aHash/4 | 1.44 ns | 1.44 ns | 485777702 |  |
| FNV1aHash/8 | 2.51 ns | 2.50 ns | 293386083 |  |
| FNV1aHash/16 | 4.97 ns | 4.97 ns | 138698088 |  |
| FNV1aHash/32 | 10.6 ns | 10.6 ns | 64218883 |  |
| FNV1aHash/64 | 27.8 ns | 27.8 ns | 25193522 |  |

### [Permutations](slugkit/benchmarks/permute_benchmark.cpp)

| Benchmark                                     | Time  |           CPU | Iterations |     |
| ----                                          | ---   | ---           | ---        | --- |
| PermutePowerOf2/1 | 3.45 ns | 3.45 ns | 202300738 | 2^1 |
| PermutePowerOf2/2 | 3.44 ns | 3.44 ns | 203448723 | 2^2 |
| PermutePowerOf2/4 | 3.44 ns | 3.44 ns | 204280305 | 2^4 |
| PermutePowerOf2/8 | 3.47 ns | 3.47 ns | 203780641 | 2^8 |
| PermutePowerOf2/16 | 3.47 ns | 3.47 ns | 200099813 | 2^16 |
| PermutePowerOf2/18 | 3.46 ns | 3.46 ns | 203496999 | 2^18 |
| Permute/1 | 1.74 ns | 1.74 ns | 400163020 | 10^1 |
| Permute/2 | 2.07 ns | 2.07 ns | 339638412 | 10^2 |
| Permute/4 | 3.06 ns | 3.06 ns | 228591396 | 10^4 |
| Permute/8 | 8.09 ns | 8.09 ns | 86644744 | 10^8 |
| Permute/16 | 16.1 ns | 16.1 ns | 42681702 | 10^16 |
| Permute/18 | 20.5 ns | 20.5 ns | 33777983 | 10^18 |

### [Parsing](slugkit/benchmarks/parse_benchmark.cpp)

| Benchmark                                     | Time  |           CPU | Iterations |     |
| ----                                          | ---   | ---           | ---        | --- |
| ParsePattern/0 | 47.3 ns | 47.3 ns | 14588729 | {number:8d} |
| ParsePattern/1 | 47.1 ns | 47.1 ns | 14897991 | {special:8} |
| ParsePattern/2 | 65.4 ns | 65.4 ns | 11137327 | {special:8-12} |
| ParsePattern/3 | 52.6 ns | 52.6 ns | 13018674 | {noun} |
| ParsePattern/4 | 52.7 ns | 52.7 ns | 13348928 | {Noun} |
| ParsePattern/5 | 52.0 ns | 52.0 ns | 13084866 | {NOUN} |
| ParsePattern/6 | 52.5 ns | 52.5 ns | 13590012 | {nOun} |
| ParsePattern/7 | 58.2 ns | 58.2 ns | 11705816 | {adjective} |
| ParsePattern/8 | 57.0 ns | 57.0 ns | 12329407 | {ADJECTIVE} |
| ParsePattern/9 | 57.1 ns | 57.1 ns | 12384193 | {Adjective} |
| ParsePattern/10 | 57.5 ns | 57.5 ns | 12138562 | {aDjective} |
| ParsePattern/11 | 96.7 ns | 96.7 ns | 7090277 | {adjective:+tag} |
| ParsePattern/12 | 133 ns | 133 ns | 5197656 | {adjective:+tag1-tag2} |
| ParsePattern/13 | 80.9 ns | 80.9 ns | 8607845 | {adjective:==10} |
| ParsePattern/14 | 158 ns | 158 ns | 4460496 | {adjective:+tag1-tag2==10} |
| ParsePattern/15 | 114 ns | 114 ns | 6402224 | {adjective}-{noun} |
| ParsePattern/16 | 171 ns | 171 ns | 4136135 | {adjective}-{noun}-{verb} |
| ParsePattern/17 | 214 ns | 214 ns | 3271819 | {adverb}-{adjective}-{noun}-{number:4x} |

### [Formatting](slugkit/benchmarks/format_pattern_benchmark.cpp)

| Benchmark                                     | Time  |           CPU | Iterations |     |
| ----                                          | ---   | ---           | ---        | --- |
| FormatPattern/1 | 17.8 ns | 17.8 ns | 38671069 | 1 components |
| FormatPattern/2 | 24.8 ns | 24.8 ns | 28105339 | 2 components |
| FormatPattern/3 | 32.7 ns | 32.7 ns | 21385705 | 3 components |
| FormatPattern/4 | 39.7 ns | 39.7 ns | 17407353 | 4 components |
| FormatPattern/5 | 47.7 ns | 47.7 ns | 14599787 | 5 components |
| FormatPattern/6 | 54.5 ns | 54.5 ns | 12422406 | 6 components |
| FormatPattern/7 | 60.8 ns | 60.8 ns | 11477934 | 7 components |
| FormatPattern/8 | 67.2 ns | 67.2 ns | 10347829 | 8 components |
| FormatPattern/9 | 74.9 ns | 74.9 ns | 9268469 | 9 components |
| FormatPattern/10 | 81.2 ns | 81.2 ns | 8572465 | 10 components |

### [Generate Numbers](slugkit/benchmarks/generate_numbers_benchmarks.cpp)

| Benchmark                                     | Time  |           CPU | Iterations |     |
| ----                                          | ---   | ---           | ---        | --- |
| GenerateHexNumbers/1 | 23.7 ns | 23.7 ns | 29600228 | number:1x |
| GenerateHexNumbers/2 | 25.8 ns | 25.8 ns | 26996569 | number:2x |
| GenerateHexNumbers/4 | 26.4 ns | 26.4 ns | 26204703 | number:4x |
| GenerateHexNumbers/8 | 27.4 ns | 27.4 ns | 25370489 | number:8x |
| GenerateHexNumbers/16 | 32.7 ns | 32.7 ns | 21690834 | number:16x |
| GenerateHexNumbersUppercase/1 | 24.4 ns | 24.4 ns | 29197958 | number:1X |
| GenerateHexNumbersUppercase/2 | 25.6 ns | 25.6 ns | 27273427 | number:2X |
| GenerateHexNumbersUppercase/4 | 26.7 ns | 26.7 ns | 26269937 | number:4X |
| GenerateHexNumbersUppercase/8 | 27.1 ns | 27.1 ns | 24933618 | number:8X |
| GenerateHexNumbersUppercase/16 | 32.3 ns | 32.3 ns | 21996090 | number:16X |
| GenerateDecNumbers/1 | 53.1 ns | 53.1 ns | 13137676 | number:1d |
| GenerateDecNumbers/2 | 62.0 ns | 62.0 ns | 11020267 | number:2d |
| GenerateDecNumbers/4 | 61.5 ns | 61.5 ns | 11267780 | number:4d |
| GenerateDecNumbers/8 | 57.8 ns | 57.8 ns | 11721555 | number:8d |
| GenerateDecNumbers/16 | 81.6 ns | 81.6 ns | 8495975 | number:16d |
| GenerateDecNumbers/18 | 95.1 ns | 95.1 ns | 7423016 | number:18d |
| GenerateRomanNumbersUppercase/1 | 18.6 ns | 18.6 ns | 37532196 | number:1R |
| GenerateRomanNumbersUppercase/2 | 14.9 ns | 14.9 ns | 46977672 | number:2R |
| GenerateRomanNumbersUppercase/4 | 12.0 ns | 12.0 ns | 59216753 | number:4R |
| GenerateRomanNumbersUppercase/8 | 32.9 ns | 32.9 ns | 21482395 | number:8R |
| GenerateRomanNumbersUppercase/15 | 25.7 ns | 25.6 ns | 27407411 | number:15R |
| GenerateRomanNumbersLowercase/1 | 65.0 ns | 65.0 ns | 10604648 | number:1r |
| GenerateRomanNumbersLowercase/2 | 61.1 ns | 61.1 ns | 11372898 | number:2r |
| GenerateRomanNumbersLowercase/4 | 65.4 ns | 65.4 ns | 10757226 | number:4r |
| GenerateRomanNumbersLowercase/8 | 104 ns | 104 ns | 6710809 | number:8r |
| GenerateRomanNumbersLowercase/15 | 96.5 ns | 96.5 ns | 7291240 | number:15r |

### [Genereate Single Words](slugkit/benchmarks/generate_from_dictionary_benchmark.cpp)

| Benchmark                                     | Time  |           CPU | Iterations |     |
| ----                                          | ---   | ---           | ---        | --- |
| GenerateFromDictionary/1000 | 193 ns | 193 ns | 3628202 |  |
| GenerateFromDictionary/10000 | 202 ns | 202 ns | 3463411 |  |
| GenerateFromDictionary/100000 | 201 ns | 201 ns | 3478886 |  |
| GenerateFromDictionary/1000000 | 354 ns | 354 ns | 2034558 |  |
| GenerateFromDictionaryUppercase/1000 | 341 ns | 341 ns | 2080563 |  |
| GenerateFromDictionaryUppercase/10000 | 346 ns | 346 ns | 2007091 |  |
| GenerateFromDictionaryUppercase/100000 | 348 ns | 348 ns | 2022727 |  |
| GenerateFromDictionaryUppercase/1000000 | 503 ns | 502 ns | 1309848 |  |
| GenerateFromDictionaryTitleCase/1000 | 6539 ns | 6539 ns | 108670 |  |
| GenerateFromDictionaryTitleCase/10000 | 6559 ns | 6559 ns | 105645 |  |
| GenerateFromDictionaryTitleCase/100000 | 6689 ns | 6689 ns | 105084 |  |
| GenerateFromDictionaryTitleCase/1000000 | 6670 ns | 6670 ns | 105205 |  |
| GenerateFromDictionaryMixedCase/1000 | 359 ns | 359 ns | 1950292 |  |
| GenerateFromDictionaryMixedCase/10000 | 402 ns | 402 ns | 1749132 |  |
| GenerateFromDictionaryMixedCase/100000 | 442 ns | 442 ns | 1370841 |  |
| GenerateFromDictionaryMixedCase/1000000 | 664 ns | 664 ns | 1033152 |  |

### [Filter Dictionaries](slugkit/benchmarks/filter_dictionary_benchmarks.cpp)

| Benchmark                                     | Time  |           CPU | Iterations |     |
| ----                                          | ---   | ---           | ---        | --- |
| FilterDictionary/0 | 237672 ns | 237658 ns | 2964 | word |
| FilterDictionary/1 | 680540 ns | 680531 ns | 1014 | word:+tag1 |
| FilterDictionary/2 | 615504 ns | 615497 ns | 1143 | word:+tag2 |
| FilterDictionary/3 | 594529 ns | 594521 ns | 1194 | word:+tag3 |
| FilterDictionary/4 | 597221 ns | 597153 ns | 1194 | word:+tag4 |
| FilterDictionary/5 | 663979 ns | 663971 ns | 1051 | word:-tag1 |
| FilterDictionary/6 | 684307 ns | 684254 ns | 1016 | word:-tag2 |
| FilterDictionary/7 | 728133 ns | 728124 ns | 947 | word:-tag3 |
| FilterDictionary/8 | 759104 ns | 759020 ns | 922 | word:-tag4 |
| FilterDictionary/9 | 1136285 ns | 1136270 ns | 612 | word:+tag1-tag2 |

### [Estimate Pattern Capacity](slugkit/benchmarks/generate_slugs_benchmarks.cpp)

| Benchmark                                     | Time  |           CPU | Iterations |     |
| ----                                          | ---   | ---           | ---        | --- |
| CalculateSettings/0 | 73940 ns | 73767 ns | 9488 | {verb}-{adverb} |
| CalculateSettings/1 | 301710 ns | 301705 ns | 2304 | {adverb}-{noun}-{verb} |
| CalculateSettings/2 | 301914 ns | 301913 ns | 2293 | {adverb}-{noun}-{verb}-{number:4x} |
| CalculateSettings/3 | 301136 ns | 301135 ns | 2311 | {adverb}-{noun}-{verb}-{adverb}-{noun}-{verb} |
| CalculateSettings/4 | 302194 ns | 302193 ns | 2306 | {adverb}-{noun}-{verb}-{adverb}-{noun}-{verb}-{adverb}-{noun}-{verb} |

### [Generate Slugs](slugkit/benchmarks/generate_slugs_benchmarks.cpp)

| Benchmark                                     | Time  |           CPU | Iterations |     |
| ----                                          | ---   | ---           | ---        | --- |
| GenerateSlugs/0 | 73533 ns | 73532 ns | 9426 | {verb}-{adverb} |
| GenerateSlugs/1 | 303032 ns | 303031 ns | 2317 | {adverb}-{noun}-{verb} |
| GenerateSlugs/2 | 301868 ns | 301867 ns | 2310 | {adverb}-{noun}-{verb}-{number:4x} |
| GenerateSlugs/3 | 613231 ns | 613229 ns | 1128 | {adverb}-{noun}-{verb}-{adverb}-{noun}-{verb} |
| GenerateSlugs/4 | 916800 ns | 916798 ns | 757 | {adverb}-{noun}-{verb}-{adverb}-{noun}-{verb}-{adverb}-{noun}-{verb} |

</details>

## Building

### Requirements

- C++20 compatible compiler
- CMake 3.20+
- [userver](https://userver.tech) framework (core components only)

### Build Instructions

> [!Caution]
> Standalone build is not there yet, slugkit directory is supposed to be added
> to a bigger CMake project with `add_subdirectory`

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

- **Core library**: `userver::core` (text utilities only)
- **Optional serialisation**: JSON/YAML that comes with userver (`userver::core`)
- **Tests**: Google Test framework

A version without userver dependencies may be provided in future releases or feel free to send me a pull request.

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
- [ ] userver-independent variant

## Licence

[Apache License 2.0](LICENSE)

## Related Projects

- [SlugKit Service](https://dev.slugkit.dev) - *Almost* production-ready API service.
- [SlugKit Python SDK](https://github.com/slugkit/slugkit-sdk) - Python SDK for using with SaaS or on-prem variants of SlugKit API service.

## Support

- **Documentation**: [TODO: Add docs link]
- **Issues**: [GitHub Issues](https://github.com/slugkit/slugkit-generator/issues)
- **Discussions**: [GitHub Discussions](https://github.com/slugkit/slugkit-generator/discussions)
