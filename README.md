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

[Benchmark results August 28, 2025](https://dev.slugkit.dev/articles/axiomatic-tutor-mcx)
[Benchmark results August 27, 2025](https://dev.slugkit.dev/articles/governing-faculty-mli)

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
