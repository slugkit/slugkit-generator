#include <slugkit/generator/permutations.hpp>

#include <userver/utils/assert.hpp>

#include <algorithm>
#include <random>

namespace slugkit::generator {

namespace {
constexpr std::uint32_t kFNV1aPrime = 0x01000193;
constexpr std::uint32_t kFNV1aOffsetBasis = 0x811c9dc5;
constexpr std::uint64_t kMulxMix64Constant = 0xff51afd7ed558ccdull;
constexpr std::uint64_t kMulxMix64Constant2 = 0xc4ceb9fe1a85ec53ull;

auto FeistelRound(std::uint32_t value, std::uint32_t key, std::uint32_t half_bits) -> std::uint32_t {
    std::uint64_t x = static_cast<std::uint64_t>(value) ^ key;
    x ^= (x >> 33);
    x *= kMulxMix64Constant;
    x ^= (x >> 33);
    x *= kMulxMix64Constant2;
    x ^= (x >> 33);
    return static_cast<std::uint32_t>(x);
}

auto LCGPermute(std::uint64_t max_value, std::uint32_t hash, std::uint64_t sequence) -> std::uint64_t {
    sequence = sequence % max_value;
    std::uint64_t multiplier = hash | 1;  // ensure multiplier is odd
    while (std::gcd(multiplier, max_value) != 1) {
        multiplier += 2;
    }
    std::uint64_t increment = (hash + 1) % max_value;
    return (multiplier * sequence + increment) % max_value;
}

auto PermuteWithHalfBits(std::uint32_t hash, std::uint32_t half_bits, std::uint64_t sequence, std::uint32_t rounds)
    -> std::uint64_t {
    std::uint64_t mask = (1ull << half_bits) - 1;
    auto l = static_cast<std::uint32_t>(sequence >> half_bits);
    auto r = static_cast<std::uint32_t>(sequence) & mask;

    for (std::uint32_t i = 0; i < rounds; ++i) {
        auto new_l = r;
        std::uint32_t f = FeistelRound(r, hash + i, half_bits) & mask;
        r = l ^ f;
        l = new_l;
    }
    return (static_cast<std::uint64_t>(l) << half_bits) | r;
}

}  // namespace

auto FNV1aHash(std::string_view str) -> std::uint32_t {
    std::uint32_t hash = kFNV1aOffsetBasis;
    for (char c : str) {
        hash ^= static_cast<std::uint8_t>(c);
        hash *= kFNV1aPrime;
    }
    return hash;
}

auto GeneratePermutation(std::uint32_t seed_value, std::size_t size, std::size_t limit) -> Permutation {
    Permutation permutation(size);
    for (std::size_t i = 0; i < size; ++i) {
        permutation[i] = i;
    }

    std::mt19937 rng(seed_value);

    // Fisher-Yates shuffle
    for (std::size_t i = size - 1; i > 0; --i) {
        std::uniform_int_distribution<std::size_t> distribution(0, i);
        auto j = distribution(rng);
        std::swap(permutation[i], permutation[j]);
    }

    if (limit == 0) {
        return permutation;
    }
    return Permutation{permutation.begin(), permutation.begin() + limit};
}

auto GeneratePermutation(std::string_view seed, std::size_t size, std::size_t limit) -> Permutation {
    auto seed_value = FNV1aHash(seed);
    return GeneratePermutation(seed_value, size, limit);
}

//-------------------------------------------------------------
// Permute with a power of two max value and a string seed
//-------------------------------------------------------------
auto PermutePowerOf2(std::uint64_t max_value, std::string_view seed, std::uint64_t sequence, std::uint32_t rounds)
    -> std::uint64_t {
    std::uint32_t hash = FNV1aHash(seed);
    return PermutePowerOf2(max_value, hash, sequence, rounds);
}

//-------------------------------------------------------------
// Permute with a power of two max value and a uint32_t seed
//-------------------------------------------------------------
auto PermutePowerOf2(std::uint64_t max_value, std::uint32_t hash, std::uint64_t sequence, std::uint32_t rounds)
    -> std::uint64_t {
    if (max_value == 0) {
        return PermutePowerOf2(hash, sequence, rounds);
    }
    UASSERT_MSG((max_value & (max_value - 1)) == 0, "FeistelPermutePowerOf2 requires max_value to be a power of two");
    if (sequence >= max_value) {
        sequence = sequence % max_value;
    }

    std::uint32_t bits = 64 - __builtin_clzll(max_value - 1);
    std::uint32_t half_bits = bits / 2;
    return PermuteWithHalfBits(hash, half_bits, sequence, rounds);
}

//-------------------------------------------------------------
// Permute with 2^64 max value and a string seed
//-------------------------------------------------------------
auto PermutePowerOf2(std::string_view seed, std::uint64_t sequence, std::uint32_t rounds) -> std::uint64_t {
    std::uint32_t hash = FNV1aHash(seed);
    return PermuteWithHalfBits(hash, 32, sequence, rounds);
}

//-------------------------------------------------------------
// Permute with 2^64 max value and a uint32_t seed
//-------------------------------------------------------------
auto PermutePowerOf2(std::uint32_t hash, std::uint64_t sequence, std::uint32_t rounds) -> std::uint64_t {
    return PermuteWithHalfBits(hash, 32, sequence, rounds);
}

//-------------------------------------------------------------
// Permute with an arbitrary max value and a string seed
//-------------------------------------------------------------
auto Permute(std::uint64_t max_value, std::string_view seed, std::uint64_t sequence, std::uint32_t rounds)
    -> std::uint64_t {
    std::uint32_t hash = FNV1aHash(seed);
    return Permute(max_value, hash, sequence, rounds);
}

//-------------------------------------------------------------
// Permute with an arbitrary max value and a uint32_t seed
//-------------------------------------------------------------
auto Permute(std::uint64_t max_value, std::uint32_t hash, std::uint64_t sequence, std::uint32_t rounds)
    -> std::uint64_t {
    if (max_value == 0) {
        return PermutePowerOf2(hash, sequence, rounds);
    }
    if (!(max_value & (max_value - 1))) {
        return PermutePowerOf2(max_value, hash, sequence, rounds);
    }
    return LCGPermute(max_value, hash, sequence);
}
}  // namespace slugkit::generator
