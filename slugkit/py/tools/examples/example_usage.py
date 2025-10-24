"""
Example usage of Pydantic models for SlugKit dictionary YAML structures.

This demonstrates the new multi-dictionary structure where a single YAML file
can contain multiple dictionaries of different kinds.
"""

import sys
from pathlib import Path

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

from slugkit.tools.models import DictionaryFile, DictionaryData, TagData


def example_create_and_save():
    """Example: Create a dictionary file with multiple dictionaries."""
    
    # Create a noun dictionary
    noun_dict = DictionaryData(
        version="1.0.0",
        description="Common English nouns",
        words={
            "en": {
                "apple": ["fruit"],
                "banana": ["fruit"],
                "car": ["vehicle"],
                "house": ["building"],
            },
            "fr": {
                "pomme": ["fruit"],
                "banane": ["fruit"],
                "voiture": ["vehicle"],
                "maison": ["building"],
            }
        },
        tags={
            "fruit": TagData(
                name="Fruit",
                description="A sweet edible plant structure",
                opt_in=False
            ),
            "vehicle": TagData(
                name="Vehicle",
                description="A means of transportation",
                opt_in=False
            ),
            "building": TagData(
                name="Building",
                description="A structure with walls and a roof",
                opt_in=False
            )
        }
    )
    
    # Create a verb dictionary
    verb_dict = DictionaryData(
        version="1.0.0",
        description="Common English verbs",
        words={
            "en": {
                "run": ["action", "movement"],
                "walk": ["action", "movement"],
                "eat": ["action", "consumption"],
            }
        },
        tags={
            "action": TagData(
                name="Action",
                description="Action verbs",
                opt_in=False
            ),
            "movement": TagData(
                name="Movement",
                description="Movement-related verbs",
                opt_in=False
            ),
            "consumption": TagData(
                name="Consumption",
                description="Consumption-related verbs",
                opt_in=False
            )
        }
    )
    
    # Create a dictionary file containing both dictionaries
    dictionary_file = DictionaryFile(dictionaries={
        "noun": noun_dict,
        "verb": verb_dict
    })
    
    # Save to YAML
    output_path = Path("/tmp/example_dictionary_file.yaml")
    dictionary_file.to_yaml(output_path)
    print(f"✅ Dictionary file saved to: {output_path}")
    
    return dictionary_file


def example_load_from_yaml():
    """Example: Load a dictionary file from YAML."""
    
    # First create a sample file
    dictionary_file = example_create_and_save()
    
    # Load it back
    loaded = DictionaryFile.from_yaml("/tmp/example_dictionary_file.yaml")
    print(f"\n✅ Dictionary file loaded!")
    print(f"   Available dictionaries: {loaded.list_dictionaries()}")
    
    # Show details for each dictionary
    for kind in loaded.list_dictionaries():
        dictionary = loaded.get_dictionary(kind)
        print(f"   {kind}: version {dictionary.version}")
        if dictionary.description:
            print(f"      Description: {dictionary.description}")
        print(f"      Languages: {list(dictionary.words.keys())}")
    
    return loaded


def example_operations():
    """Example: Various operations on dictionary files."""
    
    dictionary_file = example_create_and_save()
    
    print(f"\n🔧 Dictionary file operations:")
    
    # List all dictionaries
    kinds = dictionary_file.list_dictionaries()
    print(f"   Available dictionaries: {kinds}")
    
    # Get a specific dictionary
    noun_dict = dictionary_file.get_dictionary("noun")
    if noun_dict:
        print(f"   Noun dictionary has {len(noun_dict.words)} languages")
        
        # Get words for a language
        en_words = noun_dict.get_words_for_language("en")
        print(f"   English words in noun dict: {list(en_words.keys())}")
        
        # Get tags for a word
        apple_tags = noun_dict.get_tags_for_word("en", "apple")
        print(f"   Tags for 'apple': {apple_tags}")
    
    # Cross-dictionary operations
    all_en_words = dictionary_file.get_all_words_for_language("en")
    print(f"   All English words across dictionaries: {len(all_en_words)}")
    
    # Search by tag across all dictionaries
    fruit_words = dictionary_file.search_by_tag("en", "fruit")
    print(f"   Words tagged as 'fruit': {fruit_words}")
    
    action_words = dictionary_file.search_by_tag("en", "action")
    print(f"   Words tagged as 'action': {action_words}")


def example_modifications():
    """Example: Modifying dictionaries."""
    
    dictionary_file = example_create_and_save()
    
    print(f"\n✏️  Modifying dictionaries:")
    
    # Add words to existing dictionary
    noun_dict = dictionary_file.get_dictionary("noun")
    if noun_dict:
        noun_dict.add_word("en", "grape", ["fruit"])
        noun_dict.add_word("en", "bicycle", ["vehicle"])
        print(f"   ➕ Added words to noun dictionary")
        
        # Add a new tag
        noun_dict.add_tag(
            "tropical",
            name="Tropical",
            description="Tropical fruits",
            opt_in=True
        )
        print(f"   ➕ Added 'tropical' tag")
    
    # Create and add a new dictionary
    adjective_dict = DictionaryData(
        version="1.0.0",
        description="Common English adjectives",
        words={
            "en": {
                "big": ["size"],
                "small": ["size"],
                "red": ["color"],
                "blue": ["color"],
            }
        },
        tags={
            "size": TagData(
                name="Size",
                description="Size-related adjectives",
                opt_in=False
            ),
            "color": TagData(
                name="Color",
                description="Color-related adjectives",
                opt_in=False
            )
        }
    )
    
    dictionary_file.add_dictionary("adjective", adjective_dict)
    print(f"   ➕ Added new 'adjective' dictionary")
    
    # Show updated dictionary list
    print(f"   Updated dictionaries: {dictionary_file.list_dictionaries()}")


def example_validation():
    """Example: Validation with Pydantic."""
    
    print(f"\n🔍 Testing validation...")
    
    # Valid data - works fine
    try:
        valid_dict = DictionaryData(
            version="1.0.0",
            words={"en": {"apple": ["fruit"]}}
        )
        print("✅ Valid dictionary created")
    except Exception as e:
        print(f"❌ Error: {e}")
    
    # Invalid data - empty version
    try:
        invalid_dict = DictionaryData(
            version="",  # Empty version
            words={"en": {"apple": ["fruit"]}}
        )
        print("❌ Should have failed validation!")
    except Exception as e:
        print(f"✅ Caught validation error: {e}")
    
    # Invalid data - wrong types
    try:
        invalid_dict = DictionaryData(
            version="1.0.0",
            words="not a dict"  # Should be a dict
        )
        print("❌ Should have failed validation!")
    except Exception as e:
        print(f"✅ Caught validation error: {type(e).__name__}")


def example_building_from_scratch():
    """Example: Building a complete dictionary file from scratch."""
    
    print(f"\n🏗️  Building dictionary file from scratch:")
    
    # Create multiple dictionaries
    dictionaries = {}
    
    # Nouns
    dictionaries["noun"] = DictionaryData(
        version="1.0.0",
        description="Common nouns",
        words={
            "en": {"cat": ["animal"], "dog": ["animal"]},
            "fr": {"chat": ["animal"], "chien": ["animal"]}
        },
        tags={
            "animal": TagData(name="Animal", description="Living creatures", opt_in=False)
        }
    )
    
    # Verbs
    dictionaries["verb"] = DictionaryData(
        version="1.0.0",
        description="Common verbs",
        words={
            "en": {"run": ["action"], "walk": ["action"]},
            "fr": {"courir": ["action"], "marcher": ["action"]}
        },
        tags={
            "action": TagData(name="Action", description="Action words", opt_in=False)
        }
    )
    
    # Adjectives
    dictionaries["adjective"] = DictionaryData(
        version="1.0.0",
        description="Common adjectives",
        words={
            "en": {"big": ["size"], "small": ["size"]},
            "fr": {"grand": ["size"], "petit": ["size"]}
        },
        tags={
            "size": TagData(name="Size", description="Size-related words", opt_in=False)
        }
    )
    
    # Create the dictionary file
    dictionary_file = DictionaryFile(dictionaries=dictionaries)
    
    # Save it
    output_path = Path("/tmp/complete_dictionary.yaml")
    dictionary_file.to_yaml(output_path)
    print(f"   ✅ Created complete dictionary file: {output_path}")
    
    # Verify it loads correctly
    loaded = DictionaryFile.from_yaml(output_path)
    print(f"   ✅ Verified loading: {loaded.list_dictionaries()}")
    
    return dictionary_file


if __name__ == "__main__":
    print("=" * 60)
    print("Pydantic Multi-Dictionary Models Example Usage")
    print("=" * 60)
    
    example_create_and_save()
    example_load_from_yaml()
    example_operations()
    example_modifications()
    example_validation()
    example_building_from_scratch()
    
    print("\n" + "=" * 60)
    print("✅ All examples completed!")
    print("=" * 60)
