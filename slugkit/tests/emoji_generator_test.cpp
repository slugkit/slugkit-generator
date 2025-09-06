#include <slugkit/generator/dictionary.hpp>
#include <slugkit/generator/pattern_generator.hpp>
#include <slugkit/generator/structured_loader.hpp>

#include <userver/formats/yaml.hpp>

#include <iostream>
#include <userver/utest/utest.hpp>

namespace slugkit::generator {

using namespace literals;

UTEST(EmojiGenerator, EmbeddedData) {
    EXPECT_GT(EmojiSubstitutionGenerator::kEmojiDictionaryText.size(), 0);
}

UTEST(EmojiGenerator, EmbeddedValidYaml) {
    ASSERT_NO_THROW(userver::formats::yaml::FromString(std::string{EmojiSubstitutionGenerator::kEmojiDictionaryText}));
    auto yaml = userver::formats::yaml::FromString(std::string{EmojiSubstitutionGenerator::kEmojiDictionaryText});
    ASSERT_TRUE(yaml.IsObject());
    ASSERT_EQ(yaml.GetSize(), 1);
    ASSERT_TRUE(yaml.HasMember("emoji"));
    ASSERT_TRUE(yaml["emoji"].HasMember("words"));
    ASSERT_TRUE(yaml["emoji"]["words"].IsObject());
    ASSERT_TRUE(yaml["emoji"]["words"].HasMember("👍"));
    ASSERT_TRUE(yaml["emoji"]["words"]["👍"].IsArray());
}

UTEST(EmojiGenerator, EmbeddedDictionaryParse) {
    ASSERT_NO_THROW(userver::formats::yaml::FromString(std::string{EmojiSubstitutionGenerator::kEmojiDictionaryText}));
    auto yaml = userver::formats::yaml::FromString(std::string{EmojiSubstitutionGenerator::kEmojiDictionaryText});
    ASSERT_TRUE(yaml.HasMember("emoji"));
    ASSERT_NO_THROW(yaml["emoji"].As<data::Dictionary<std::unordered_set<std::string>>>());
    auto data = yaml["emoji"].As<data::Dictionary<std::unordered_set<std::string>>>();
    EXPECT_TRUE(data.language.empty());
    EXPECT_FALSE(data.words.empty());
    EXPECT_GT(data.words.size(), 0);
    std::cerr << "Emoji dictionary size: " << data.words.size() << std::endl;
    std::size_t max_symbol_length = 0;
    std::string max_symbol;
    for (const auto& item : data.words) {
        max_symbol_length = std::max(max_symbol_length, item.word.size());
        max_symbol = item.word;
    }
    std::cerr << "Max symbol length: " << max_symbol_length << std::endl;
    std::cerr << "Max symbol: " << max_symbol << std::endl;
}

UTEST(EmojiGenerator, GetTagDefinitions) {
    auto tag_definitions = kEmojiDictionary.GetTagDefinitions();
    EXPECT_GT(tag_definitions.size(), 0);
}

UTEST(EmojiGenerator, Generate) {
    EmojiSubstitutionGenerator generator{"emoji"_emoji_gen};
    auto seed_hash = PatternGenerator::SeedHash("test");
    EXPECT_EQ(generator.Generate(seed_hash, 0), "🐋");
    EXPECT_EQ(generator.Generate(seed_hash, 1), "🏂");
    EXPECT_EQ(generator.Generate(seed_hash, 2), "❤️‍🔥");
    EXPECT_EQ(generator.Generate(seed_hash, 3), "\xF0\x9F\x87\xB2\xF0\x9F\x87\xA6");
    EXPECT_EQ(generator.Generate(seed_hash, 4), "😏");
}

UTEST(EmojiGenerator, GenerateFaces) {
    EmojiSubstitutionGenerator generator{"emoji:+face"_emoji_gen};
    auto seed_hash = PatternGenerator::SeedHash("test");
    EXPECT_EQ(generator.GetMaxLength(), 1);
    EXPECT_EQ(generator.GetCapacity(), 112);
    EXPECT_EQ(generator.Generate(seed_hash, 0), "🤣");
    EXPECT_EQ(generator.Generate(seed_hash, 1), "😊");
    EXPECT_EQ(generator.Generate(seed_hash, 2), "😘");
    EXPECT_EQ(generator.Generate(seed_hash, 3), "😋");
    EXPECT_EQ(generator.Generate(seed_hash, 4), "🤑");
}

UTEST(EmojiGenerator, GenerateFaceSetNonUnique) {
    {
        EmojiSubstitutionGenerator generator{"emoji:+face count=2"_emoji_gen};
        auto seed_hash = PatternGenerator::SeedHash("test");
        EXPECT_EQ(generator.GetMaxLength(), 2);
        EXPECT_EQ(generator.GetCapacity(), 12544);
        EXPECT_EQ(generator.Generate(seed_hash, 0), "🤣☺️");
        EXPECT_EQ(generator.Generate(seed_hash, 1), "😊🙄");
        EXPECT_EQ(generator.Generate(seed_hash, 2), "😘😷");
        EXPECT_EQ(generator.Generate(seed_hash, 3), "😋😎");
        EXPECT_EQ(generator.Generate(seed_hash, 4), "🤑🥴");
    }
    {
        EmojiSubstitutionGenerator generator{"emoji:+face count=6"_emoji_gen};
        auto seed_hash = PatternGenerator::SeedHash("test");
        EXPECT_EQ(generator.GetMaxLength(), 6);
        EXPECT_EQ(generator.GetCapacity(), 1973822685184);
        EXPECT_EQ(generator.Generate(seed_hash, 0), "🤣☺️🤮🥹☺️😀");
        EXPECT_EQ(generator.Generate(seed_hash, 1), "😊🙄😆🤒😬😀");
        EXPECT_EQ(generator.Generate(seed_hash, 2), "😘😷😵🤑🤕😀");
        EXPECT_EQ(generator.Generate(seed_hash, 3), "😋😎🙂🙉🧐😀");
        EXPECT_EQ(generator.Generate(seed_hash, 4), "🤑🥴😧🫡💩😀");
    }
    {
        EmojiSubstitutionGenerator generator{"emoji:+face count=0-6"_emoji_gen};
        auto seed_hash = PatternGenerator::SeedHash("test");
        EXPECT_EQ(generator.GetMaxLength(), 6);
        EXPECT_EQ(generator.GetCapacity(), 1991604871537);
        EXPECT_EQ(generator.Generate(seed_hash, 0), "🤣☺️🤮🥹☺️");
        EXPECT_EQ(generator.Generate(seed_hash, 1), "😊🙄😆🤒😬");
        EXPECT_EQ(generator.Generate(seed_hash, 2), "😘😷😵🤑🤕");
        EXPECT_EQ(generator.Generate(seed_hash, 3), "😋😎🙂🙉🧐");
        EXPECT_EQ(generator.Generate(seed_hash, 4), "🤑🥴😧🫡💩");
    }
}

UTEST(EmojiGenerator, GenerateFaceSetUnique) {
    {
        EmojiSubstitutionGenerator generator{"emoji:+face count=2 unique=true"_emoji_gen};
        auto seed_hash = PatternGenerator::SeedHash("test");
        EXPECT_EQ(generator.GetMaxLength(), 2);
        EXPECT_EQ(generator.GetCapacity(), 12432);
        EXPECT_EQ(generator.Generate(seed_hash, 0), "😧🤓");
        EXPECT_EQ(generator.Generate(seed_hash, 1), "😋😶");
        EXPECT_EQ(generator.Generate(seed_hash, 2), "😶‍🌫️😼");
        EXPECT_EQ(generator.Generate(seed_hash, 3), "☹️😯");
        EXPECT_EQ(generator.Generate(seed_hash, 4), "🙈😜");
    }
    {
        EmojiSubstitutionGenerator generator{"emoji:+face count=6 unique=true"_emoji_gen};
        auto seed_hash = PatternGenerator::SeedHash("test");
        EXPECT_EQ(generator.GetMaxLength(), 6);
        EXPECT_EQ(generator.GetCapacity(), 1722533662080);
        EXPECT_EQ(generator.Generate(seed_hash, 0), "😀😛😶🫤😖🤯");
        EXPECT_EQ(generator.Generate(seed_hash, 1), "😀😣😦🤒😶‍🌫️😗");
        EXPECT_EQ(generator.Generate(seed_hash, 2), "😀😯👽🤗😗🫤");
        EXPECT_EQ(generator.Generate(seed_hash, 3), "😀🤨😗🙉🥵😔");
        EXPECT_EQ(generator.Generate(seed_hash, 4), "😀😿😭🫡😹😽");
    }
    {
        EmojiSubstitutionGenerator generator{"emoji:+face count=0-6 unique=true"_emoji_gen};
        auto seed_hash = PatternGenerator::SeedHash("test");
        EXPECT_EQ(generator.GetMaxLength(), 6);
        EXPECT_EQ(generator.GetCapacity(), 1738782547265);
        EXPECT_EQ(generator.Generate(seed_hash, 0), "😙🤮👾😺☺️");
        EXPECT_EQ(generator.Generate(seed_hash, 1), "🙁😆😪😮‍💨😣");
        EXPECT_EQ(generator.Generate(seed_hash, 2), "🥶🤯😎😈😧");
        EXPECT_EQ(generator.Generate(seed_hash, 3), "🫥🙃🤮🤯😵‍💫");
        EXPECT_EQ(generator.Generate(seed_hash, 4), "😹😮😖😠😄");
    }
}

}  // namespace slugkit::generator
