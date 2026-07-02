"""
Pydantic models for SlugKit dictionary YAML structures.

The structure of the YAML file is:
[kind]:
  version: string
  description: string (optional)
  words:
    [language]:
      [word]: [tag, ...] # word to tags mapping for the language
  tags: # tags definition, may be omitted or supplied in another file
    [tag]:
      name: string # the name of the tag
      description: string # the description of the tag
      opt_in: boolean # whether the tag is opt-in

A YAML file can contain multiple dictionaries, each with a different kind as the top-level key.
"""

from typing import Dict, List, Optional
from pathlib import Path

import yaml
from pydantic import BaseModel, Field, field_validator


class TagData(BaseModel):
    """Tag metadata for dictionary words."""
    
    name: str = Field(..., description="The name of the tag")
    description: str = Field(..., description="The description of the tag")
    opt_in: bool = Field(False, description="Whether the tag is opt-in")

    class Config:
        frozen = False  # Allow mutation if needed
        extra = "forbid"  # Reject unknown fields


class DictionaryData(BaseModel):
    """Single dictionary data structure."""
    
    version: str = Field(..., description="The version of the dictionary")
    description: Optional[str] = Field(None, description="Description of the dictionary")
    case_mutation: bool = Field(
        True,
        description=(
            "Whether to generate lower/upper/title case variants for each word. "
            "Set false for a verbatim dictionary (e.g. a fixed copy corpus for a game): "
            "words are stored as-is and every selector case yields the original text."
        ),
    )
    words: Dict[str, Dict[str, List[str]]] = Field(
        ...,
        description="Mapping of language codes to word data objects"
    )
    tags: Optional[Dict[str, TagData]] = Field(
        None,
        description="Mapping of tag names to tag data objects"
    )
    
    class Config:
        extra = "forbid"
    
    @field_validator("version")
    @classmethod
    def validate_version(cls, v: str) -> str:
        """Ensure version is non-empty."""
        if not v or not v.strip():
            raise ValueError("Version cannot be empty")
        return v
    
    def get_words_for_language(self, language: str) -> Dict[str, List[str]]:
        """Get all words for a specific language."""
        return self.words.get(language, {})
    
    def get_tags_for_word(self, language: str, word: str) -> List[str]:
        """Get all tags for a specific word in a language."""
        return self.words.get(language, {}).get(word, [])
    
    def add_word(self, language: str, word: str, tags: List[str]) -> None:
        """Add a word with tags to a language."""
        if language not in self.words:
            self.words[language] = {}
        self.words[language][word] = tags
        for tag in tags:
            self.register_tag(tag)
    
    def register_tag(self, tag_id: str, name: str = "", description: str = "", opt_in: bool = False) -> None:
        """Register a tag definition."""
        if self.tags is None:
            self.tags = {}
        if tag_id not in self.tags:
            self.tags[tag_id] = TagData(name=name, description=description, opt_in=opt_in)

    def add_tag(self, tag_id: str, name: str, description: str, opt_in: bool = False) -> None:
        """Add a tag definition."""
        if self.tags is None:
            self.tags = {}
        self.tags[tag_id] = TagData(name=name, description=description, opt_in=opt_in)

    def merge(self, other: "DictionaryData") -> None:
        """Merge another dictionary's words and tags into this one (used to compile a kind whose
        source is split across multiple YAML files). Words present in both are unioned by tag;
        tag definitions already present win (first file is authoritative)."""
        for language, words in other.words.items():
            target = self.words.setdefault(language, {})
            for word, tags in words.items():
                if word in target:
                    target[word] = sorted(set(target[word]) | set(tags))
                else:
                    target[word] = tags
        if other.tags:
            if self.tags is None:
                self.tags = {}
            for tag_id, tag_data in other.tags.items():
                self.tags.setdefault(tag_id, tag_data)


class DictionaryFile(BaseModel):
    """Container for multiple dictionaries in a single YAML file."""
    
    dictionaries: Dict[str, DictionaryData] = Field(
        ..., 
        description="Mapping of dictionary kinds to dictionary data"
    )
    
    class Config:
        extra = "forbid"
    
    @classmethod
    def from_yaml(cls, file_path: str | Path) -> "DictionaryFile":
        """Load multiple dictionaries from a YAML file."""
        with open(file_path, 'r', encoding='utf-8') as f:
            data = yaml.safe_load(f)
        
        if not isinstance(data, dict):
            raise ValueError("YAML file must contain a dictionary")
        
        # Convert each dictionary kind to DictionaryData
        dictionaries = {}
        for kind, dict_data in data.items():
            if not isinstance(dict_data, dict):
                raise ValueError(f"Dictionary '{kind}' must be a dictionary")
            dictionaries[kind] = DictionaryData(**dict_data)
        
        return cls(dictionaries=dictionaries)
    
    def to_yaml(self, file_path: str | Path) -> None:
        """Save multiple dictionaries to a YAML file."""
        data = {}
        for kind, dictionary in self.dictionaries.items():
            data[kind] = dictionary.model_dump(exclude_none=True)
        
        with open(file_path, 'w', encoding='utf-8') as f:
            yaml.dump(
                data,
                f,
                allow_unicode=True,
                default_flow_style=False,
                sort_keys=False
            )
    
    def get_dictionary(self, kind: str) -> Optional[DictionaryData]:
        """Get a specific dictionary by kind."""
        return self.dictionaries.get(kind)
    
    def add_dictionary(self, kind: str, dictionary: DictionaryData) -> None:
        """Add a new dictionary."""
        self.dictionaries[kind] = dictionary

    def merge(self, other: "DictionaryFile") -> None:
        """Merge another DictionaryFile into this one, per kind (see DictionaryData.merge)."""
        for kind, data in other.dictionaries.items():
            if kind in self.dictionaries:
                self.dictionaries[kind].merge(data)
            else:
                self.dictionaries[kind] = data
    
    def list_dictionaries(self) -> List[str]:
        """List all available dictionary kinds."""
        return list(self.dictionaries.keys())
    
    def get_all_words_for_language(self, language: str) -> Dict[str, List[str]]:
        """Get all words for a language across all dictionaries."""
        all_words = {}
        for dictionary in self.dictionaries.values():
            all_words.update(dictionary.get_words_for_language(language))
        return all_words
    
    def search_by_tag(self, language: str, tag: str) -> List[str]:
        """Find all words with a specific tag across all dictionaries."""
        results = []
        for dictionary in self.dictionaries.values():
            words = dictionary.get_words_for_language(language)
            for word, tags in words.items():
                if tag in tags:
                    results.append(word)
        return results

