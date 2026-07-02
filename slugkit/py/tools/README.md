# SlugKit Tools

Python utilities for building and managing SlugKit dictionaries.

## Overview

`slugkit.tools` provides the **`compile-dict`** command-line tool plus Pydantic models for creating, loading, and manipulating YAML dictionary files used by the SlugKit C++ generator. `compile-dict` turns dictionary YAML into the binary `.bin` format the generator memory-maps for efficient use in production.

## Installation

From the repository root:

```bash
# Install in development mode
pip install -e libs/generator/slugkit/py/tools
```

Or using uv:

```bash
uv pip install -e libs/generator/slugkit/py/tools
```

## Compiling dictionaries

Installing the package provides the `compile-dict` command, which compiles dictionary YAML into the binary `.bin` files the C++ generator loads:

```bash
# Compile a file; writes one <base>.<kind>.bin per kind found in the YAML
compile-dict nouns.yaml -o nouns.yaml            # -> nouns.noun.bin, nouns.verb.bin, ...

# Split a large dictionary across several files, merged per kind
compile-dict geo.part1.yaml geo.part2.yaml -o geo.yaml   # -> geo.city.bin, ...
```

| Argument | Meaning |
|----------|---------|
| `input_files` (1+) | Dictionary YAML file(s). Multiple inputs are **merged per kind**, so one dictionary's source can be split across files (e.g. `geo.part1.yaml` + `geo.part2.yaml`). |
| `-o, --output <base>` | Output base path. Each *kind* in the input is written to its own file, named `<base with .yaml stripped>.<kind>.bin` — **one binary per kind**, containing all of that kind's languages. |

Load the resulting `.bin` files with the generator's `binary::DictionarySet` (or the C ABI / Swift·Kotlin·Dart·Flutter bindings): add one file per kind. The on-disk layout is described in [`docs/binary_dictionary_format.md`](../../../docs/binary_dictionary_format.md).

## Quick Start (Python API)

```python
from slugkit.tools import DictionaryFile, DictionaryData

# Load a YAML file containing multiple dictionaries
dictionary_file = DictionaryFile.from_yaml("dictionaries.yaml")

# Get a specific dictionary (e.g., noun, verb, adjective)
noun_dict = dictionary_file.get_dictionary("noun")

# Query words and tags
words = noun_dict.get_words_for_language("en")
tags = noun_dict.get_tags_for_word("en", "apple")

# Add new content
noun_dict.add_word("en", "grape", ["fruit"])
noun_dict.add_tag("berry", name="Berry", description="Small fruit", opt_in=False)

# Save changes
dictionary_file.to_yaml("updated.yaml")
```

## YAML Format

Each top-level key is a **kind** (`noun`, `verb`, `colour`, `corpus`, …); a single file may hold several. A kind maps each language to a `word → [tags]` table:

```yaml
noun:
  version: "1.0.0"                 # required
  description: "Common English nouns"  # optional
  case_mutation: true              # optional, default true (see below)
  words:
    en:                            # language code (ISO 639-1), or "" for language-agnostic
      apple: [fruit, food]         # word -> list of tags (may be empty: [])
      banana: [fruit, food, tropical]
    fr:
      pomme: [fruit, food]
      banane: [fruit, food, tropical]
  tags:                            # optional; per-tag metadata
    fruit:
      name: Fruit
      description: A sweet edible plant structure
      opt_in: false               # default false
```

### Dictionary fields

| Field | Required | Default | Meaning |
|-------|----------|---------|---------|
| `version` | yes | — | Non-empty version string, stored in the binary header. |
| `description` | no | `null` | Human-readable description. |
| `case_mutation` | no | `true` | Whether to precompute lower/upper/title case variants. Set `false` for a verbatim dictionary (below). |
| `words` | yes | — | `language → { word → [tags] }`. A word's tag list may be empty. |
| `tags` | no | `{}` | Metadata for the tags used above (`name`, `description`, `opt_in`). Tags need not be declared to be used, but declaring them sets `opt_in`. |

### Tags and opt-in

A tag can be marked **`opt_in: true`**. Words carrying an opt-in tag are **hidden by default** in the generator: they appear only when a selector requests that tag explicitly (`{noun:+nsfw}`) or the tag is enabled at runtime (`Generator::EnableOptIn`). Use it for content like `nsfw` that should be off unless asked for.

### Verbatim dictionaries (`case_mutation: false`)

By default each word is stored with precomputed lowercase, uppercase and title-case variants, and the selector's kind-case picks one (`{noun}`, `{NOUN}`, `{Noun}`, `{nOun}`). Set `case_mutation: false` to store words **exactly as written** and emit them verbatim for *any* selector case — useful for a fixed copy corpus (e.g. a game) where casing is meaningful:

```yaml
corpus:
  version: "1.0.0"
  case_mutation: false
  words:
    en:
      iPhone: []
      "Game Over": []
```

Here `{corpus}`, `{Corpus}` and `{CORPUS}` all yield `iPhone` / `Game Over` unchanged (mixed-case counts each word as a single form).

## Models

### DictionaryFile

Container for multiple dictionaries in a single YAML file.

**Key Methods:**
- `from_yaml(path)` - Load from YAML
- `to_yaml(path)` - Save to YAML
- `get_dictionary(kind)` - Get specific dictionary (noun, verb, etc.)
- `list_dictionaries()` - List all dictionary kinds
- `search_by_tag(lang, tag)` - Find words across all dictionaries

### DictionaryData

Represents a single dictionary (noun, verb, adjective, etc.).

**Key Methods:**
- `get_words_for_language(lang)` - Get words for a language
- `get_tags_for_word(lang, word)` - Get tags for a word
- `add_word(lang, word, tags)` - Add a new word
- `add_tag(tag_id, name, desc, opt_in)` - Add a tag definition

### TagData

Tag metadata with name, description, and opt-in flag.

## Examples

See the `examples/` directory:

```bash
# Run the sample dictionary demo
python examples/test_sample_dictionary.py

# Run various usage examples
python examples/example_usage.py
```

## Building Dictionaries from Data Sources

### From NLTK WordNet

```python
import nltk
from slugkit.tools import DictionaryFile, DictionaryData, TagData

nltk.download('wordnet')

noun_dict = DictionaryData(
    version="1.0.0",
    description="NLTK WordNet nouns",
    words={"en": {}},
    tags={}
)

from nltk.corpus import wordnet
for synset in wordnet.all_synsets('n'):
    word = synset.lemmas()[0].name().replace('_', ' ')
    noun_dict.add_word("en", word, ["nltk", "wordnet"])

# Save to YAML
dictionary_file = DictionaryFile(dictionaries={"noun": noun_dict})
dictionary_file.to_yaml("nltk_nouns.yaml")
```

### From BabelNet

Similar pattern - iterate through your BabelNet data and use `add_word()` and `add_tag()`.

## Integration with C++ Generator

Compile these YAML dictionaries into binary `.bin` files with `compile-dict` (see [Compiling dictionaries](#compiling-dictionaries)), then load them with the C++ generator:

- **Memory-mapped**: the generator maps the `.bin` files rather than copying them
- **Language-specific access**: fast lookup by language code (ISO 639-1)
- **Tag-based filtering**: efficient filtering by tags, including hidden opt-in tags

The on-disk layout is documented in [`docs/binary_dictionary_format.md`](../../../docs/binary_dictionary_format.md).

## Language Codes

- Use ISO 639-1 two-letter codes: `en`, `fr`, `es`, `de`, etc.
- Use the **empty string** `""` for language-agnostic content (e.g. domain names, TLDs). A selector with no `@lang` resolves to `en` when present, otherwise falls back to the agnostic (`""`) words.

## Development

```bash
# Install with dev dependencies
pip install -e "libs/generator/slugkit/py/tools[dev]"

# Run tests (when available)
pytest

# Type checking
mypy src/slugkit/tools
```

## License

See project LICENSE file.

