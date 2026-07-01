#include <slugkit/generator/permutations.hpp>

#include <userver/utest/utest.hpp>

namespace slugkit::generator {

UTEST(FNV1aHash, Empty) {
    EXPECT_EQ(FNV1aHash(""), 0x811c9dc5);
}

UTEST(FNV1aHash, Basic) {
    EXPECT_EQ(FNV1aHash("test"), 0xafd071e5);

    EXPECT_EQ(FNV1aHash("test"), FNV1aHash("test"));
    EXPECT_NE(FNV1aHash("test"), FNV1aHash("test2"));
}

template <typename T>
void CheckUniqueness(std::uint64_t max_value, std::function<T(std::uint64_t)> permute) {
    std::set<T> values;
    for (std::uint64_t i = 0; i < max_value; ++i) {
        values.insert(permute(i));
    }
    EXPECT_EQ(values.size(), max_value);
}

template <typename T>
void CheckStability(std::function<T(std::uint64_t)> permute) {
    for (std::uint64_t i = 0; i < 1000; ++i) {
        EXPECT_EQ(permute(i), permute(i));
    }
}

template <typename T>
void CheckDistinct(std::function<T(std::uint64_t)> permute_a, std::function<T(std::uint64_t)> permute_b) {
    for (std::uint64_t i = 0; i < 1000; ++i) {
        EXPECT_NE(permute_a(i), permute_b(i));
    }
}

UTEST(PermutationGen, PermutePowerOfTwoUniqueness) {
    std::uint64_t max_value = 0x10000;
    CheckUniqueness<std::uint64_t>(max_value, [max_value](std::uint64_t i) {
        return PermutePowerOf2(max_value, "test", i);
    });
}

UTEST(PermutationGen, PermuteSmallHexNumbers) {
    CheckUniqueness<std::uint64_t>(16, [](std::uint64_t i) { return Permute(16, "test", i); });
}

UTEST(PermutationGen, PermutePowerOfTwoStability) {
    EXPECT_EQ(PermutePowerOf2(0x10000, "test", 0), 24690);
    EXPECT_EQ(PermutePowerOf2(0x10000, "test", 1), 12039);
    EXPECT_EQ(PermutePowerOf2(0x10000, "test", 0xffff), 47821);
    EXPECT_EQ(PermutePowerOf2(0x10000, "test", 0x10000), 24690);
    EXPECT_EQ(PermutePowerOf2(0x10000, "test", 0x10001), 12039);
}

UTEST(PermutationGen, PermuteUintMaxValue) {
    EXPECT_EQ(Permute(0, "test", 0), 15223634918887569383ULL);
    EXPECT_EQ(Permute(0, "test", 1), 12220871165612158745ULL);
    EXPECT_EQ(Permute(0, "test", 0xffff), 4470626935250758252ULL);
    EXPECT_EQ(Permute(0, "test", 0x10000), 18416224773763666316ULL);
    EXPECT_EQ(Permute(0, "test", 0x10001), 10061440996843264425ULL);
}

UTEST(PermutationGen, PermuteUniqueness) {
    std::uint64_t max_value = 100000;
    CheckUniqueness<std::uint64_t>(max_value, [max_value](std::uint64_t i) {
        return Permute(max_value, "test", i, 4);
    });
}

UTEST(PermutationGen, PermuteUniquenessPrime) {
    std::uint64_t max_value = 92503;
    CheckUniqueness<std::uint64_t>(max_value, [max_value](std::uint64_t i) {
        return Permute(max_value, "test", i, 4);
    });
}

UTEST(PermutationGen, PermuteUniqueUniqueness) {
    std::size_t alphabet_size = 10;
    std::size_t sequence_length = 5;
    auto unique_permutation_count = UniquePermutationCount(alphabet_size, sequence_length);
    std::cout << "unique_permutation_count: " << unique_permutation_count << std::endl;
    CheckUniqueness<std::vector<std::uint64_t>>(
        unique_permutation_count,
        [alphabet_size, sequence_length](std::size_t i) { return UniquePermutation(alphabet_size, sequence_length, i); }
    );
}

UTEST(PermutationGen, PermuteUniqueStability) {
    std::size_t alphabet_size = 10;
    std::size_t sequence_length = 5;
    CheckStability<std::vector<std::uint64_t>>([alphabet_size, sequence_length](std::size_t i) {
        return UniquePermutation(alphabet_size, sequence_length, i);
    });
}

UTEST(PermutationGen, PermuteUniqueSeedHashUniqueness) {
    std::size_t alphabet_size = 10;
    std::size_t sequence_length = 5;
    auto unique_permutation_count = UniquePermutationCount(alphabet_size, sequence_length);
    auto seed_hash = FNV1aHash("test");
    CheckUniqueness<std::vector<std::uint64_t>>(
        unique_permutation_count,
        [alphabet_size, sequence_length, seed_hash](std::size_t i) {
            return UniquePermutation(seed_hash, alphabet_size, sequence_length, i);
        }
    );
}

UTEST(PermutationGen, PermuteUniqueSeedHashStability) {
    std::size_t alphabet_size = 10;
    std::size_t sequence_length = 5;
    auto seed_hash = FNV1aHash("test");
    CheckStability<std::vector<std::uint64_t>>([alphabet_size, sequence_length, seed_hash](std::size_t i) {
        return UniquePermutation(seed_hash, alphabet_size, sequence_length, i);
    });
}

UTEST(PermutationGen, PermuteUniqueSeedHashDistinct) {
    std::size_t alphabet_size = 10;
    std::size_t sequence_length = 5;
    auto seed_hash_a = FNV1aHash("test");
    auto seed_hash_b = FNV1aHash("test2");
    CheckDistinct<std::vector<std::uint64_t>>(
        [alphabet_size, sequence_length, seed_hash_a](std::size_t i) {
            return UniquePermutation(seed_hash_a, alphabet_size, sequence_length, i);
        },
        [alphabet_size, sequence_length, seed_hash_b](std::size_t i) {
            return UniquePermutation(seed_hash_b, alphabet_size, sequence_length, i);
        }
    );
}

UTEST(PermutationGen, PermuteNonUniqueUniqueness) {
    std::size_t alphabet_size = 10;
    std::size_t sequence_length = 5;
    auto non_unique_permutation_count = PermutationCount(alphabet_size, sequence_length);
    std::cout << "non_unique_permutation_count: " << non_unique_permutation_count << std::endl;
    auto seed_hash = FNV1aHash("test");
    CheckUniqueness<std::vector<std::uint64_t>>(
        non_unique_permutation_count,
        [alphabet_size, sequence_length, seed_hash](std::size_t i) {
            return NonUniquePermutation(seed_hash, alphabet_size, sequence_length, i);
        }
    );
}

// Regression: for ranges above 2^32 the permutation must mix the high bits, so consecutive sequence
// numbers change the most-significant part of the output. Previously LCGPermute clustered them (the
// output moved by only ~multiplier < 2^32 per step), which e.g. kept the first emoji of
// {emoji:count=5 unique=true} fixed for hundreds of sequences.
UTEST(PermutationGen, PermuteLargeRangeMixesHighBits) {
    constexpr std::uint64_t kMaxValue = 2028900546777600ULL;  // UniquePermutationCount(1150, 5)
    constexpr std::uint64_t kBucket = kMaxValue / 1150;       // width of the leading "digit"

    std::set<std::uint64_t> high_digits;
    for (std::uint64_t i = 0; i < 16; ++i) {
        auto value = Permute(kMaxValue, "aaa", i);
        EXPECT_LT(value, kMaxValue);
        EXPECT_EQ(value, Permute(kMaxValue, "aaa", i));  // deterministic
        high_digits.insert(value / kBucket);
    }
    // Before the fix this was 1 (the leading digit never changed); now it should be ~16.
    EXPECT_GE(high_digits.size(), 8u);
}

// The cycle-walking permutation for large non-power-of-two ranges relies on PermutePowerOf2 being a
// bijection over its 2^bits domain -- including ODD bit widths (the enclosing domain of the
// emoji-unique capacity is 51 bits). The Feistel network is only bijective over an odd-width domain
// with an EVEN round count, so this guards the default (kDefaultRounds = 4).
UTEST(PermutationGen, PermutePowerOfTwoBijectionOddAndEven) {
    for (std::uint32_t bits : {5u, 7u, 8u, 9u, 12u, 13u}) {
        std::uint64_t domain = 1ull << bits;
        CheckUniqueness<std::uint64_t>(domain, [domain](std::uint64_t i) {
            return PermutePowerOf2(domain, "test", i);
        });
    }
}

UTEST(PermutationGen, PermuteLargeRangeSampledUniqueness) {
    constexpr std::uint64_t kMaxValue = 5000000000ULL;  // > 2^32, not a power of two
    std::set<std::uint64_t> values;
    for (std::uint64_t i = 0; i < 2000; ++i) {
        auto value = Permute(kMaxValue, "test", i, 4);
        EXPECT_LT(value, kMaxValue);
        values.insert(value);
    }
    EXPECT_EQ(values.size(), 2000u);  // collision-free over the sample (still a bijection)
}

}  // namespace slugkit::generator
