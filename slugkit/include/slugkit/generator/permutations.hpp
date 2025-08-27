#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

namespace slugkit::generator {

using Permutation = std::vector<std::size_t>;

std::uint32_t FNV1aHash(std::string_view str);

Permutation GeneratePermutation(std::uint32_t seed, std::size_t size, std::size_t limit = 0);
Permutation GeneratePermutation(std::string_view seed, std::size_t size, std::size_t limit = 0);

using PermutationGenerator = std::function<Permutation(std::string_view seed, std::size_t size, std::size_t limit)>;

constexpr std::uint32_t kDefaultRounds = 4;

/// @brief Permutation with a max value (uses Feistel permutation for power of two max values or LCG for other values).
/// @note The domain of the permutation is [0, max_value).
/// @param max_value The maximum value for the permutation.
/// @param seed The seed for the permutation.
/// @param sequence The sequence number for the permutation.
/// @param rounds The number of rounds for the permutation (default 4).
auto Permute(
    std::uint64_t max_value,
    std::string_view seed,
    std::uint64_t sequence,
    std::uint32_t rounds = kDefaultRounds
) -> std::uint64_t;

/// @brief Permutation with a max value (uses Feistel permutation for power of two max values or LCG for other values).
/// @note The domain of the permutation is [0, max_value).
/// @param max_value The maximum value for the permutation.
/// @param seed The seed for the permutation.
/// @param sequence The sequence number for the permutation.
/// @param rounds The number of rounds for the permutation (default 4).
auto Permute(std::uint64_t max_value, std::uint32_t seed, std::uint64_t sequence, std::uint32_t rounds = kDefaultRounds)
    -> std::uint64_t;

/// @brief Permutation with a power of two max value (uses Feistel permutation).
/// @note The domain of the permutation is [0, max_value).
/// @param max_value The maximum value for the permutation.
/// @param seed The seed for the permutation.
/// @param sequence The sequence number for the permutation.
/// @param rounds The number of rounds for the permutation (default 4).
/// @return The permutation.
auto PermutePowerOf2(
    std::uint64_t max_value,
    std::string_view seed,
    std::uint64_t sequence,
    std::uint32_t rounds = kDefaultRounds
) -> std::uint64_t;
auto PermutePowerOf2(
    std::uint64_t max_value,
    std::uint32_t seed_hash,
    std::uint64_t sequence,
    std::uint32_t rounds = kDefaultRounds
) -> std::uint64_t;

/// @brief Permutation with a power of two max value (uses Feistel permutation).
/// @note The domain of the permutation is [0, 2^64).
/// @param seed The seed for the permutation.
/// @param sequence The sequence number for the permutation.
/// @param rounds The number of rounds for the permutation (default 4).
/// @return The permutation.
auto PermutePowerOf2(std::string_view seed, std::uint64_t sequence, std::uint32_t rounds = kDefaultRounds)
    -> std::uint64_t;
auto PermutePowerOf2(std::uint32_t seed_hash, std::uint64_t sequence, std::uint32_t rounds = kDefaultRounds)
    -> std::uint64_t;
}  // namespace slugkit::generator
