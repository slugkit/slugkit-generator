#pragma once
#include <cstdint>

namespace slugkit::generator::constants {

// pattern length limits
constexpr auto kMaxDecimalLength = 18U;   // 18 digits, fits in 64 bits signed integer
constexpr auto kMaxHexLength = 16U;       // 16 hex digits (8 bytes)
constexpr auto kMaxSpecialLength = 12U;   // 12 special symbols ~ 5 bits per symbol, 60 bits total
constexpr auto kCpuRelaxIterations = 10;  // number of iterations for CPU relaxation while generating slugs,
                                          // 10 is a good default for most cases
constexpr auto kMaxEmojiCount = 6U;

// base cost for a dictionary generator
constexpr auto kDictionaryBaseCost = 5;
// cost for a tag in a dictionary generator
constexpr auto kDictionaryTagCost = 2;
// cost for the limiting length of a dictionary generator
constexpr auto kDictionaryLengthCost = 2;
// cost for the tag and the limiting length both present in a dictionary generator
constexpr auto kDictionaryTagAndLengthCost = 1;
// cost for the case of a dictionary generator
constexpr auto kDictionaryUpperCaseCost = 2;
constexpr auto kDictionaryTitleCaseCost = 3;
constexpr auto kDictionaryMixedCaseCost = 6;

// base cost for a number generator
constexpr auto kNumberBaseCost = 3;

// base cost for a special character generator
constexpr auto kSpecialCharBaseCost = 4;
// cost for the limiting length of a special character generator (over 2 symbols)
constexpr auto kSpecialCharLengthCost = 1;
// cost for variable length of a special character generator
constexpr auto kSpecialCharVariableLengthCost = 2;

// base cost for an emoji generator
constexpr auto kEmojiBaseCost = 5;
// cost for the limiting count of an emoji generator
constexpr auto kEmojiCountCost = 1;
// cost for the include/exclude tag of an emoji generator
constexpr auto kEmojiTagsCost = 2;
// cost for the unique flag of an emoji generator
constexpr auto kEmojiUniqueCost = 5;

// maximum number of chars in an emoji (known to be 20)
constexpr auto kEmojiMaxCharLength = 20;

}  // namespace slugkit::generator::constants
