#pragma once

#include <slugkit/utils/numeric.hpp>

namespace slugkit::generator {
// ---------------------------------------------------------------------------------------------------------------------
/// @brief Statistics for a dictionary.
struct DictionaryStats {
    std::string kind;
    std::string language;
    std::int64_t count;
};

// ---------------------------------------------------------------------------------------------------------------------
/// @brief Definition of a tag.
struct TagDefinition {
    std::string kind;
    std::string tag;
    std::optional<std::string> description;
    bool opt_in;
    std::int32_t word_count;
};

// ---------------------------------------------------------------------------------------------------------------------
/// @brief Settings for the generation of human-readable IDs.
/// This settings are calculated for a given pattern, to maximize
/// the capacity of the generator. Selected size may be different
/// from the original size, because the original size may not be a prime number.
/// @note If selecting a prime number doesn't improve the capacity,
///       the original size is selected.
struct SelectorSettings {
    std::int64_t original_size;
    std::int64_t selected_size;
};

/// @brief Settings for the generation of human-readable IDs.
/// This settings are calculated for a given pattern, to maximize
/// the capacity of the generator.
/// To ensure stable generation, the capacity must be stored along with the pattern
/// and used for generation.
struct PatternSettings {
    /// @brief The selectors settings for the pattern.
    /// Settings are stored for selectors only, number generators are skipped.
    std::vector<SelectorSettings> selectors;
    // TODO: remove capacity and max_pattern_length from here
    numeric::BigInt capacity;
    std::int32_t max_pattern_length;
};

/// @brief Information about a pattern.
// struct PatternInfo : PatternSettings {
//     numeric::BigInt capacity;
//     std::int32_t max_pattern_length;
// };

}  // namespace slugkit::generator
