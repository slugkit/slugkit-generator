#pragma once

#include <slugkit/generator/binary_dictionary.hpp>
#include <slugkit/generator/dictionary.hpp>
#include <slugkit/generator/generator.hpp>
#include <slugkit/generator/pattern.hpp>

#include <slugkit/compat/fast_pimpl.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace slugkit::generator {

/// @brief Substitution generator that generates a string from a placeholder
/// @note Viable only while the seed string is in memory, so the pattern generator
/// will own this one.
class SubstitutionGenerator {
public:
    virtual ~SubstitutionGenerator() = default;

    /// @param sequence_number The sequence number to generate a string for
    /// @return The generated string
    virtual std::string Generate(std::uint32_t seed, std::size_t sequence_number) const = 0;
    virtual numeric::BigInt GetCapacity() const = 0;
    virtual std::size_t GetMaxLength() const = 0;
};

using SubstitutionGeneratorPtr = std::unique_ptr<SubstitutionGenerator>;

/// @brief Substitution generator that uses a selector to choose a substitution
/// Caches the filtered dictionary for the selector and the permutation
/// @param selector The selector to use
/// @param settings The settings to use
/// @param seed The seed to use
class SelectorSubstitutionGenerator : public SubstitutionGenerator {
public:
    SelectorSubstitutionGenerator(FilteredDictionaryConstPtr dictionary, const SelectorSettings& settings);
    ~SelectorSubstitutionGenerator() override = default;

    std::string Generate(std::uint32_t seed, std::size_t sequence_number) const override;

    numeric::BigInt GetCapacity() const override {
        return numeric::BigInt(selected_size_);
    }
    std::size_t GetMaxLength() const override;

private:
    FilteredDictionaryConstPtr dictionary_;
    std::int64_t selected_size_;
};

/// @brief Selector substitution generator backed by a memory-mapped binary dictionary.
/// Functionally identical to SelectorSubstitutionGenerator but consumes binary::WordEntry
/// precomputed case variants (string_views) instead of the std::string-returning in-memory
/// FilteredDictionary, so it produces byte-identical slugs without per-word case conversion
/// (kMixed uses the lowercase variant plus a per-word case mask).
class BinarySelectorSubstitutionGenerator : public SubstitutionGenerator {
public:
    BinarySelectorSubstitutionGenerator(binary::FilteredDictionaryConstPtr dictionary, const SelectorSettings& settings);
    ~BinarySelectorSubstitutionGenerator() override = default;

    std::string Generate(std::uint32_t seed, std::size_t sequence_number) const override;

    numeric::BigInt GetCapacity() const override {
        return numeric::BigInt(selected_size_);
    }
    std::size_t GetMaxLength() const override;

private:
    binary::FilteredDictionaryConstPtr dictionary_;
    std::int64_t selected_size_;
};

/// @brief Substitution generator that uses a number generator to generate a string
/// @note Can be used for any number base, but not for roman numerals
/// @param number_gen The number generator to use
/// @param seed The seed to use
class NumberSubstitutionGenerator : public SubstitutionGenerator {
public:
    NumberSubstitutionGenerator(const NumberGen& number_gen);
    ~NumberSubstitutionGenerator() override = default;

    std::string Generate(std::uint32_t seed, std::size_t sequence_number) const override;

    numeric::BigInt GetCapacity() const override;
    std::size_t GetMaxLength() const override {
        return max_length_;
    }

private:
    NumberBase base_;
    std::uint32_t max_length_;
};

struct FilteredRomanDictionary;
using FilteredRomanDictionaryPtr = std::shared_ptr<FilteredRomanDictionary>;

/// @brief Substitution generator that uses a roman numeral generator to generate a string
/// @note Can be used only for roman numerals
/// @param number_gen The number generator to use
/// @param seed The seed to use
class RomanSubstitutionGenerator : public SubstitutionGenerator {
public:
    RomanSubstitutionGenerator(const NumberGen& number_gen);
    ~RomanSubstitutionGenerator() override = default;

    std::string Generate(std::uint32_t seed, std::size_t sequence_number) const override;

    numeric::BigInt GetCapacity() const override;
    std::size_t GetMaxLength() const override {
        return max_length_;
    }

private:
    FilteredRomanDictionaryPtr roman_dictionary_;
    NumberBase base_;
    std::uint32_t max_length_;
};

/// @brief Substitution generator that generates a string of special symbols
class SpecialSubstitutionGenerator : public SubstitutionGenerator {
public:
    SpecialSubstitutionGenerator(const SpecialCharGen& special_gen);
    ~SpecialSubstitutionGenerator() override = default;

    std::string Generate(std::uint32_t seed, std::size_t sequence_number) const override;

    numeric::BigInt GetCapacity() const override;
    std::size_t GetMaxLength() const override {
        return max_length_;
    }

private:
    std::size_t SelectLength(std::uint32_t seed, std::size_t sequence_number) const;

    std::uint32_t min_length_;
    std::uint32_t max_length_;
    std::vector<std::size_t> cumulative_caps_;
};

/// @brief Shared logic for emoji generation, independent of the dictionary backing. The emoji
/// dictionary is now a regular kind sourced from the (binary or in-memory) DictionarySet rather
/// than an embedded global; only the per-index emoji lookup differs between the two, so it is the
/// single virtual hook (EmojiAt). Count selection, permutations and capacity are shared.
class EmojiSubstitutionGeneratorBase : public SubstitutionGenerator {
public:
    EmojiSubstitutionGeneratorBase(std::size_t dict_size, const EmojiGen& emoji_gen);
    ~EmojiSubstitutionGeneratorBase() override = default;

    std::string Generate(std::uint32_t seed, std::size_t sequence_number) const override;

    numeric::BigInt GetCapacity() const override;
    /// @return The maximum length of the generated string
    /// @note This is not the max number if chars, but the max number of emoji
    std::size_t GetMaxLength() const override {
        return static_cast<std::size_t>(max_count_);
    }

protected:
    /// @brief Return the emoji string at @p index in the filtered dictionary.
    virtual std::string EmojiAt(std::size_t index) const = 0;

    std::size_t dict_size_;

private:
    std::size_t SelectCount(std::uint32_t seed, std::size_t sequence_number) const;

    std::size_t min_count_;
    std::size_t max_count_;
    bool unique_;
    std::string_view tone_;
    std::string_view gender_;
    std::vector<std::size_t> cumulative_caps_;
};

/// @brief Emoji generator over an in-memory filtered dictionary.
class EmojiSubstitutionGenerator : public EmojiSubstitutionGeneratorBase {
public:
    EmojiSubstitutionGenerator(FilteredDictionaryConstPtr dictionary, const EmojiGen& emoji_gen)
        : EmojiSubstitutionGeneratorBase(dictionary->size(), emoji_gen)
        , dictionary_{std::move(dictionary)} {}

protected:
    std::string EmojiAt(std::size_t index) const override {
        return (*dictionary_)[index];
    }

private:
    FilteredDictionaryConstPtr dictionary_;
};

/// @brief Emoji generator over a binary (memory-mapped) filtered dictionary.
class BinaryEmojiSubstitutionGenerator : public EmojiSubstitutionGeneratorBase {
public:
    BinaryEmojiSubstitutionGenerator(binary::FilteredDictionaryConstPtr dictionary, const EmojiGen& emoji_gen)
        : EmojiSubstitutionGeneratorBase(dictionary->size(), emoji_gen)
        , dictionary_{std::move(dictionary)} {}

protected:
    std::string EmojiAt(std::size_t index) const override {
        return std::string{(*dictionary_)[IndexType(static_cast<IndexType::UnderlyingType>(index))].Lowercase()};
    }

private:
    binary::FilteredDictionaryConstPtr dictionary_;
};

//-------------------------------------------------------------
// PatternGenerator
//-------------------------------------------------------------
/// @brief Pattern generator that generates strings from a pattern
/// Meant to be reused for multiple sequence numbers or to be cached.
/// Creates an array of substitution generators for each placeholder
/// in the pattern.
/// @param dictionaries The dictionaries to use
/// @param settings The settings to use
/// @param pattern The pattern to use
/// @param seed The seed to use
class PatternGenerator {
public:
    PatternGenerator(const DictionarySet& dictionaries, PatternPtr pattern);
    PatternGenerator(const DictionarySet& dictionaries, PatternPtr pattern, PatternSettings settings);
    PatternGenerator(const binary::DictionarySet& dictionaries, PatternPtr pattern);
    PatternGenerator(const binary::DictionarySet& dictionaries, PatternPtr pattern, PatternSettings settings);
    ~PatternGenerator();

    /// @brief Generate a string from a pattern
    /// @param seed The seed to use
    /// @param sequence_number The sequence number to generate a string for
    /// @return The generated string
    std::string operator()(std::uint32_t seed, std::size_t sequence_number) const;

    /// @brief Generate a string from a pattern
    /// @param seed The seed to use
    /// @param sequence_number The sequence number to generate a string for
    /// @return The generated string
    std::string operator()(std::string_view seed, std::size_t sequence_number) const;

    numeric::BigInt GetCapacity() const;
    std::size_t GetMaxPatternLength() const;
    const PatternSettings& GetSettings() const;

    static std::uint32_t SeedHash(std::string_view seed);

private:
    // Impl size differs by toolchain/stdlib (userver/gcc+libstdc++ vs standalone clang+libc++),
    // so the pimpl storage is sized per build.
#ifdef SLUGKIT_USE_USERVER
    static constexpr std::size_t kPimplSize = 160UL;
    static constexpr std::size_t kPimplAlign = 16UL;
#else
    static constexpr std::size_t kPimplSize = 144UL;
    static constexpr std::size_t kPimplAlign = 16UL;
#endif
    struct Impl;
    slugkit::compat::FastPimpl<Impl, kPimplSize, kPimplAlign> impl_;
};

}  // namespace slugkit::generator
