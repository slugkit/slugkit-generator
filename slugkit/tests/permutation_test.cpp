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

UTEST(PermutationGen, GeneratePermutation) {
    auto permutation = GeneratePermutation("test", 10);
    EXPECT_EQ(permutation.size(), 10);

    permutation = GeneratePermutation("test", 10, 5);
    EXPECT_EQ(permutation.size(), 5);

    auto permutation2 = GeneratePermutation("test", 10, 5);
    EXPECT_EQ(permutation, permutation2);
}

UTEST(PermutationGen, GenerateBigPermutation) {
    auto permutation = GeneratePermutation("test", 10000);
    EXPECT_EQ(permutation.size(), 10000);

    auto permutation2 = GeneratePermutation("test", 10000);
    EXPECT_EQ(permutation, permutation2);
}

// UTEST(PermutationGen, FeistelPermute) {
//     EXPECT_EQ(FeistelPermute(10000, "test", 0), 9383);
//     EXPECT_EQ(FeistelPermute(10000, "test", 1), 8745);
//     EXPECT_EQ(FeistelPermute(10000, "test", 9999), 3194);
//     EXPECT_EQ(FeistelPermute(10000, "test", 10000), 9383);
//     EXPECT_EQ(FeistelPermute(10000, "test", 10001), 8745);
// }

void CheckUniqueness(std::uint64_t max_value, std::function<std::uint64_t(std::uint64_t)> permute) {
    std::set<std::uint64_t> values;
    for (std::uint64_t i = 0; i < max_value; ++i) {
        values.insert(permute(i));
    }
    EXPECT_EQ(values.size(), max_value);
}

UTEST(PermutationGen, PermutePowerOfTwoUniqueness) {
    std::uint64_t max_value = 0x10000;
    CheckUniqueness(max_value, [max_value](std::uint64_t i) { return PermutePowerOf2(max_value, "test", i); });
}

UTEST(PermutationGen, PermuteSmallHexNumbers) {
    CheckUniqueness(16, [](std::uint64_t i) { return Permute(16, "test", i); });
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
    CheckUniqueness(max_value, [max_value](std::uint64_t i) { return Permute(max_value, "test", i, 4); });
}

UTEST(PermutationGen, PermuteUniquenessPrime) {
    std::uint64_t max_value = 92503;
    CheckUniqueness(max_value, [max_value](std::uint64_t i) { return Permute(max_value, "test", i, 4); });
}

}  // namespace slugkit::generator
