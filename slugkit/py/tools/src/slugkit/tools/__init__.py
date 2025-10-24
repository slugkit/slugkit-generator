"""
SlugKit Tools - Python utilities for building and managing SlugKit dictionaries.

For working with dictionary YAML files, import the models directly:

Example:
    from slugkit.tools.models import DictionaryFile, DictionaryData
    
    # Load a file with multiple dictionaries
    dictionary_file = DictionaryFile.from_yaml("path/to/dict.yaml")
    
    # Get a specific dictionary
    noun_dict = dictionary_file.get_dictionary("noun")
    words = noun_dict.get_words_for_language("en")
"""

__version__ = "0.1.0"

# Export the main models for convenience
from .models import (
    DictionaryFile,
    DictionaryData,
    TagData,
)

__all__ = [
    "DictionaryFile",
    "DictionaryData", 
    "TagData",
]

