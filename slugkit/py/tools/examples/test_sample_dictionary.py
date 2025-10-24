"""
Test script for loading and working with the sample dictionary.

This demonstrates real-world usage with an actual dictionary file.
"""

import sys
from pathlib import Path

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

from slugkit.tools.models import DictionaryFile, DictionaryData, TagData


def main():
    """Load and explore the sample dictionary."""

    # Get the sample dictionary path
    sample_file = Path(__file__).parent / "sample_dictionary.yaml"

    print("=" * 70)
    print("LOADING SAMPLE DICTIONARY")
    print("=" * 70)
    print(f"\n📁 Loading: {sample_file}")

    # Load the dictionary file
    dictionary_file = DictionaryFile.from_yaml(sample_file)

    print(f"\n✅ Successfully loaded dictionary file!")
    print(f"   Available dictionaries: {', '.join(dictionary_file.list_dictionaries())}")

    # Show details for each dictionary
    for kind in dictionary_file.list_dictionaries():
        dictionary = dictionary_file.get_dictionary(kind)
        print(f"\n📚 Dictionary: {kind}")
        print(f"   Version: {dictionary.version}")
        if dictionary.description:
            print(f"   Description: {dictionary.description}")

        # Show languages
        print(f"   Languages: {', '.join(dictionary.words.keys())}")

        # Show word counts per language
        print(f"   Word counts:")
        for lang, words in dictionary.words.items():
            print(f"      {lang}: {len(words)} words")

        # Show tags
        if dictionary.tags:
            print(f"   Tags ({len(dictionary.tags)}):")
            for tag_id, tag_data in dictionary.tags.items():
                opt_in_marker = "🔒" if tag_data.opt_in else "🔓"
                print(f"      {opt_in_marker} {tag_id}: {tag_data.name}")
                print(f"         └─ {tag_data.description}")

    # Show some English words from the noun dictionary
    noun_dict = dictionary_file.get_dictionary("noun")
    if noun_dict:
        print(f"\n🍎 Sample English words from noun dictionary:")
        en_words = noun_dict.get_words_for_language("en")
        for word, tags in list(en_words.items())[:5]:
            print(f"   {word}: {', '.join(tags)}")

        # Show translations for "apple"
        print(f"\n🌐 Translations for 'apple':")
        translations = {"en": "apple", "fr": "pomme", "es": "manzana"}
        for lang, word in translations.items():
            tags = noun_dict.get_tags_for_word(lang, word)
            print(f"   {lang}: {word} → {tags}")

        # Filter tropical fruits
        print(f"\n🌴 Tropical fruits in English:")
        for word, tags in en_words.items():
            if "tropical" in tags and "fruit" in tags:
                print(f"   🥭 {word}")

        # Statistics
        print(f"\n📈 Statistics for noun dictionary:")
        total_words = sum(len(words) for words in noun_dict.words.values())
        print(f"   Total words across all languages: {total_words}")

        # Count words per tag in English
        tag_counts = {}
        for word, tags in en_words.items():
            for tag in tags:
                tag_counts[tag] = tag_counts.get(tag, 0) + 1

        print(f"   English words by tag:")
        for tag, count in sorted(tag_counts.items(), key=lambda x: -x[1]):
            print(f"      {tag}: {count} words")

    # Demonstrate modification
    print(f"\n✏️  Demonstrating modifications:")

    # Add a new word to the noun dictionary
    if noun_dict:
        noun_dict.add_word("en", "pineapple", ["fruit", "tropical"])
        print(f"   ➕ Added 'pineapple' to English noun dictionary")

        # Add a new tag
        noun_dict.add_tag("exotic", name="Exotic", description="Uncommon or exotic foods", opt_in=True)
        print(f"   ➕ Added 'exotic' tag to noun dictionary")

    # Create a new verb dictionary
    verb_dict = DictionaryData(
        version="1.0.0",
        description="Common English verbs",
        words={
            "en": {
                "run": ["action", "movement"],
                "walk": ["action", "movement"],
                "eat": ["action", "consumption"],
                "drink": ["action", "consumption"],
            }
        },
        tags={
            "action": TagData(name="Action", description="Action verbs", opt_in=False),
            "movement": TagData(name="Movement", description="Movement-related verbs", opt_in=False),
            "consumption": TagData(name="Consumption", description="Consumption-related verbs", opt_in=False),
        },
    )

    # Add the verb dictionary to the file
    dictionary_file.add_dictionary("verb", verb_dict)
    print(f"   ➕ Added new 'verb' dictionary")

    # Show cross-dictionary search
    print(f"\n🔍 Cross-dictionary search:")
    all_en_words = dictionary_file.get_all_words_for_language("en")
    print(f"   Total English words across all dictionaries: {len(all_en_words)}")

    # Search for words with "fruit" tag across all dictionaries
    fruit_words = dictionary_file.search_by_tag("en", "fruit")
    print(f"   Words tagged as 'fruit' across all dictionaries: {fruit_words}")

    # Save to a new file
    output_file = Path("/tmp/modified_dictionary.yaml")
    dictionary_file.to_yaml(output_file)
    print(f"   💾 Saved modified dictionary file to: {output_file}")

    print(f"\n{'='*70}")
    print("✅ ALL OPERATIONS COMPLETED SUCCESSFULLY!")
    print("=" * 70)

    # Show how to use it programmatically
    print(f"\n💡 Example usage in your code:")
    print(
        f"""
    from slugkit.tools.models import DictionaryFile, DictionaryData
    
    # Load dictionary file
    dictionary_file = DictionaryFile.from_yaml("path/to/dict.yaml")
    
    # Get a specific dictionary
    noun_dict = dictionary_file.get_dictionary("noun")
    
    # Get all English fruits from noun dictionary
    for word, tags in noun_dict.get_words_for_language("en").items():
        if "fruit" in tags:
            print(f"{{word}} is a fruit")
    
    # Search across all dictionaries
    all_fruits = dictionary_file.search_by_tag("en", "fruit")
    
    # Add new dictionary
    new_dict = DictionaryData(version="1.0.0", words={{"en": {{"test": ["tag"]}}}})
    dictionary_file.add_dictionary("adjective", new_dict)
    
    # Save changes
    dictionary_file.to_yaml("modified.yaml")
    """
    )


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback

        traceback.print_exc()
        sys.exit(1)
