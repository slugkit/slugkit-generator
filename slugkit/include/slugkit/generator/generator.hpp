#pragma once

#include <slugkit/generator/dictionary.hpp>
#include <slugkit/generator/dictionary_types.hpp>
#include <slugkit/generator/permutations.hpp>

#include <slugkit/utils/numeric.hpp>

#include <userver/utils/fast_pimpl.hpp>

#include <map>
#include <string>
#include <vector>

namespace slugkit::generator {

using GenerateCallback = std::function<void(std::string generated_slug)>;

/// @brief A generator is a class that generates human-readable IDs.
/// TODO move to a separate file
class Generator {
public:
    Generator(DictionarySet dictionaries);
    ~Generator() noexcept;

    [[nodiscard]] auto RandomSeed() const -> std::string;

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
    constexpr static std::size_t kImplSize = 96;
    constexpr static std::size_t kImplAlign = 8;
    struct Impl;
    userver::utils::FastPimpl<Impl, kImplSize, kImplAlign> impl_;
};

using DictionaryStatistics = std::vector<DictionaryStats>;
using TagDefinitions = std::vector<TagDefinition>;

}  // namespace slugkit::generator
