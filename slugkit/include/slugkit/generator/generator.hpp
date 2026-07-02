#pragma once

#include <slugkit/generator/dictionary.hpp>
#include <slugkit/generator/dictionary_types.hpp>
#include <slugkit/generator/permutations.hpp>

#include <slugkit/utils/numeric.hpp>

#include <slugkit/compat/fast_pimpl.hpp>

#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace slugkit::generator {

namespace binary {
class DictionarySet;
}  // namespace binary

using GenerateCallback = std::function<void(std::string generated_slug)>;

/// @brief A generator is a class that generates human-readable IDs.
/// TODO move to a separate file
class Generator {
public:
    /// @brief Construct from an in-memory dictionary set (YAML/JSON-loaded).
    Generator(DictionarySet dictionaries);
    /// @brief Construct from a binary (memory-mapped) dictionary set. Produces byte-identical
    /// slugs to the in-memory set built from the same data.
    Generator(binary::DictionarySet dictionaries);
    ~Generator() noexcept;

    [[nodiscard]] auto RandomSeed() const -> std::string;

    //@{
    /// @name Opt-in tags ("honest opt-ins")
    /// Tags flagged opt-in in the dictionary are hidden by default: a word carrying one is only
    /// produced when a selector requests that tag explicitly (e.g. `{noun:+nsfw}`) or when the
    /// tag has been enabled here. This is a per-generator usage flag, not part of the pattern
    /// grammar. Enabling a tag lifts its gate without restricting output to it (unlike `+tag`).
    /// Enabling changes which words a pattern can produce and therefore its capacity, so treat
    /// these as configuration to be set before generation (not concurrently with it).

    /// @brief Lift the opt-in gate for a single tag.
    void EnableOptIn(std::string_view tag);
    /// @brief Replace the set of enabled opt-in tags.
    void SetEnabledOptIns(std::vector<std::string> tags);
    /// @brief Disable all opt-in tags (restore the default: every opt-in tag hidden).
    void ClearOptIns();
    /// @brief The currently enabled opt-in tags (sorted).
    [[nodiscard]] auto EnabledOptIns() const -> std::vector<std::string>;
    //@}

    /// @brief Calculates the maximum capacity and settings for a given pattern.
    /// @param pattern The pattern to calculate the capacity for.
    /// @return The capacity for the given pattern and settings to
    /// achieve maximum capacity.
    [[nodiscard]] auto GetCapacity(std::string_view pattern) const -> PatternSettings;
    [[nodiscard]] auto GetCapacity(PatternPtr pattern) const -> PatternSettings;

    //@{
    /// @name Single-shot generation

    /// @brief Generates a human-readable ID based on the pattern, seed, and sequence number.
    /// @ingroup Internal parsing
    /// @param pattern The pattern to use for generation.
    /// @param seed The seed to use for generation.
    /// @param sequence_number The sequence number to use for generation.
    /// @return The generated human-readable ID.
    [[nodiscard]] auto Generate(std::string_view pattern, std::string_view seed, std::size_t sequence_number) const
        -> std::string;

    /// @brief Generates a human-readable ID based on the settings, pattern, seed, and sequence number.
    /// @ingroup External parsing
    /// @param settings The settings to use for generation.
    /// @param pattern The pattern to use for generation.
    /// @param seed The seed to use for generation.
    /// @param sequence_number The sequence number to use for generation.
    /// @return The generated human-readable ID.
    [[nodiscard]] auto Generate(
        const PatternSettings& settings,
        PatternPtr pattern,
        std::string_view seed,
        std::size_t sequence_number
    ) const -> std::string;

    /// @brief Generates a human-readable ID based on the pattern, seed, and sequence number.
    /// @ingroup External settings
    /// @param pattern Parsed pattern to use for generation.
    /// @param seed The seed to use for generation.
    /// @param sequence_number The sequence number to use for generation.
    /// @return The generated human-readable ID.
    [[nodiscard]] auto Generate(PatternPtr pattern, std::string_view seed, std::size_t sequence_number) const
        -> std::string;
    //@}

    //@{
    /// @name Batch generation

    /// @brief Generates a human-readable ID based on the pattern, seed, and sequence number.
    /// @ingroup Internal parsing
    /// Parsers the pattern, calculates the settings, and generates the IDs.
    /// @param pattern Parsed pattern to use for generation.
    /// @param seed The seed to use for generation.
    /// @param sequence_number The sequence number to use for generation.
    /// @param count The number of IDs to generate.
    /// @param callback The callback to call for each generated ID.
    void Generate(
        std::string_view pattern,
        std::string_view seed,
        std::size_t sequence_number,
        std::size_t count,
        GenerateCallback callback
    ) const;

    /// @brief Generates a human-readable ID based on the parsed pattern, seed, and sequence number.
    /// @ingroup External parsing
    /// Calculates the settings, and generates the IDs.
    /// @param pattern Parsed pattern to use for generation.
    /// @param seed The seed to use for generation.
    /// @param sequence_number The sequence number to use for generation.
    /// @param count The number of IDs to generate.
    /// @param callback The callback to call for each generated ID.
    void Generate(
        PatternPtr pattern,
        std::string_view seed,
        std::size_t sequence_number,
        std::size_t count,
        GenerateCallback callback
    ) const;

    /// @brief Generates a human-readable ID based on the settings, parsed pattern, seed, and sequence number.
    /// @ingroup External settings
    /// @param settings The settings to use for generation.
    /// @param pattern Parsed pattern to use for generation.
    /// @param seed The seed to use for generation.
    /// @param sequence_number The sequence number to use for generation.
    /// @param count The number of IDs to generate.
    /// @param callback The callback to call for each generated ID.
    void Generate(
        const PatternSettings& settings,
        PatternPtr pattern,
        std::string_view seed,
        std::size_t sequence_number,
        std::size_t count,
        GenerateCallback callback
    ) const;

    //@}

    /// @brief Syntactic sugar for Generate.
    /// @param args The arguments to pass to Generate.
    template <typename... Args>
    [[nodiscard]] auto operator()(Args&&... args) const {
        return Generate(std::forward<Args>(args)...);
    }

private:
    // Holds a variant of the in-memory and binary dictionary sets.
    // Impl size differs by toolchain/stdlib (userver/gcc+libstdc++ vs standalone clang+libc++),
    // so the pimpl storage is sized per build.
#ifdef SLUGKIT_USE_USERVER
    static constexpr std::size_t kPimplSize = 144UL;
    static constexpr std::size_t kPimplAlign = 8UL;
#else
    static constexpr std::size_t kPimplSize = 56UL;
    static constexpr std::size_t kPimplAlign = 8UL;
#endif
    struct Impl;
    slugkit::compat::FastPimpl<Impl, kPimplSize, kPimplAlign> impl_;

    // Enabled opt-in tags (owning). Kept outside the pimpl so adding it never disturbs the
    // per-toolchain pimpl sizing. A std::set node's string address is stable, so a TagSet view
    // built from these entries stays valid for the duration of a filter call.
    std::set<std::string> enabled_opt_ins_;
};

using DictionaryStatistics = std::vector<DictionaryStats>;
using TagDefinitions = std::vector<TagDefinition>;

}  // namespace slugkit::generator
