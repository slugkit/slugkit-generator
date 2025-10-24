# SlugKit Tools

Python utilities for building and managing SlugKit dictionaries.

## Overview

`slugkit.tools` provides Pydantic models for creating, loading, and manipulating YAML dictionary files used by the SlugKit C++ generator. These dictionaries will be compiled into binary formats (memory-mapped or fully-loaded) for efficient use in production.

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

## Quick Start

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

Dictionary YAML files support multiple dictionaries per file:

```yaml
noun:
  version: "1.0.0"
  description: "Common English nouns"
  words:
    en:
      apple: [fruit, food]
      banana: [fruit, food, tropical]
    fr:
      pomme: [fruit, food]
      banane: [fruit, food, tropical]
  tags:
    fruit:
      name: Fruit
      description: A sweet edible plant structure
      opt_in: false

verb:
  version: "1.0.0"
  description: "Common English verbs"
  words:
    en:
      run: [action, movement]
      walk: [action, movement]
  tags:
    action:
      name: Action
      description: Action verbs
      opt_in: false
```

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

These YAML dictionaries are designed to be compiled into binary dictionaries for the C++ SlugKit generator:

- **Memory-mapped dictionaries**: Efficient loading of large datasets
- **Fully-loaded dictionaries**: Complete dictionaries in memory
- **Language-specific access**: Fast lookup by language code (ISO 639-1)
- **Tag-based filtering**: Efficient filtering by tags

The exact binary format is TBD.

## Language Codes

- Use ISO 639-1 two-letter codes: `en`, `fr`, `es`, `de`, etc.
- Use `agnostic` for language-agnostic content (e.g., domain names, TLDs)

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

