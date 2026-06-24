#include <slugkit/generator/filter/filtered_ids.hpp>
#include <slugkit/generator/filter/filtered_ids_io.hpp>

#include <userver/utest/utest.hpp>

namespace slugkit::generator::filter {

using namespace literals;

UTEST(IndexRange, Empty) {
    IndexRange range;
    EXPECT_TRUE(range.empty());
    EXPECT_EQ(range.size(), 0);
    EXPECT_EQ(range.range(), 0);
    EXPECT_EQ(range.density(), 0.0);
    EXPECT_EQ(range.front(), 0);
}

UTEST(IndexRange, Invalid) {
    EXPECT_THROW(IndexRange(1_idx, 0_idx), std::invalid_argument);
}

UTEST(IndexRange, Single) {
    IndexRange range(1_idx, 11_idx);
    EXPECT_FALSE(range.empty());
    EXPECT_EQ(range.size(), 1);
    EXPECT_EQ(range.range(), 10);
    EXPECT_FLOAT_EQ(range.density(), 0.1);
    EXPECT_EQ(range.front(), 1);
    EXPECT_EQ(range.back(), 10);
}

UTEST(IndexRange, IndexAccess) {
    IndexRange range(1_idx, 11_idx);
    EXPECT_EQ(range.at(0), 1);
    EXPECT_EQ(range.at(1), 2);
    EXPECT_THROW(range.at(11), std::out_of_range);
}

UTEST(IndexRange, Format) {
    IndexRange range(1_idx, 11_idx);
    EXPECT_EQ(fmt::format("{}", range), "[1, 11)");
}

UTEST(IndexRange, Intersection) {
    IndexRange range1(1_idx, 11_idx);
    IndexRange range2(6_idx, 16_idx);
    EXPECT_TRUE(range1 & range2);
    EXPECT_EQ(range1 * range2, IndexRange(6_idx, 11_idx));
}

UTEST(IndexRange, NoIntersection) {
    IndexRange range1(1_idx, 11_idx);
    IndexRange range2(16_idx, 26_idx);
    EXPECT_FALSE(range1 & range2);
    EXPECT_EQ(range1 * range2, IndexRange());
}

UTEST(IndexRange, UnionOverlapping) {
    {
        IndexRange range1(1_idx, 11_idx);
        IndexRange range2(6_idx, 16_idx);
        EXPECT_EQ(range1 + range2, IndexRangeSequence(1_idx, 16_idx));
    }
    {
        IndexRange range1(1_idx, 6_idx);
        IndexRange range2(6_idx, 16_idx);
        EXPECT_EQ(range1 + range2, IndexRangeSequence(1_idx, 16_idx));
    }
}

UTEST(IndexRange, UnionNonOverlapping) {
    IndexRange range1(1_idx, 11_idx);
    IndexRange range2(16_idx, 26_idx);
    EXPECT_EQ(range1 + range2, IndexRangeSequence({{1_idx, 11_idx}, {16_idx, 26_idx}}));
}

UTEST(IndexRangeSequence, Empty) {
    IndexRangeSequence sequence;
    EXPECT_TRUE(sequence.empty());
    EXPECT_EQ(sequence.size(), 0);
    EXPECT_EQ(sequence.range(), 0);
    EXPECT_EQ(sequence.density(), 0.0);
}

UTEST(IndexRangeSequence, Format) {
    IndexRangeSequence sequence({{1_idx, 11_idx}, {16_idx, 26_idx}});
    EXPECT_EQ(fmt::format("{}", sequence), "{[1, 11), [16, 26)}");
}

UTEST(IndexRangeSequence, IndexAccess) {
    IndexRangeSequence sequence({{1_idx, 11_idx}, {16_idx, 26_idx}});
    EXPECT_EQ(sequence.front(), 1);
    EXPECT_EQ(sequence.back(), 25);
    EXPECT_EQ(sequence.size(), 2);
    EXPECT_EQ(sequence.range(), 24);
    EXPECT_EQ(sequence.count(), 20);
    EXPECT_EQ(sequence.at(0), 1);
    EXPECT_EQ(sequence.at(1), 2);
    EXPECT_EQ(sequence.at(9), 10);
    EXPECT_EQ(sequence.at(10), 16);
    EXPECT_EQ(sequence.at(11), 17);
    EXPECT_EQ(sequence.at(12), 18);

    EXPECT_THROW(sequence.at(21), std::out_of_range);
}

UTEST(IndexRangeSequence, IntersectRangeOverlapping) {
    IndexRangeSequence sequence({{1_idx, 11_idx}, {16_idx, 26_idx}});
    IndexRange range(6_idx, 17_idx);
    EXPECT_TRUE(sequence & range);
    EXPECT_EQ(sequence * range, IndexRangeSequence({{6_idx, 11_idx}, {16_idx, 17_idx}}));
    EXPECT_EQ(range * sequence, IndexRangeSequence({{6_idx, 11_idx}, {16_idx, 17_idx}}));
}

UTEST(IndexRangeSequence, IntersectRangeNonOverlapping) {
    IndexRangeSequence sequence({{1_idx, 11_idx}, {16_idx, 26_idx}});
    IndexRange range(27_idx, 37_idx);
    EXPECT_FALSE(sequence & range);
    EXPECT_EQ(sequence * range, IndexRangeSequence());
    EXPECT_EQ(range * sequence, IndexRangeSequence());
}

UTEST(IndexRangeSequence, IntersectSequenceOverlapping) {
    IndexRangeSequence sequence1({{1_idx, 11_idx}, {16_idx, 28_idx}});
    IndexRangeSequence sequence2({{6_idx, 17_idx}, {27_idx, 37_idx}});
    EXPECT_TRUE(sequence1 & sequence2);
    EXPECT_EQ(sequence1 * sequence2, IndexRangeSequence({{6_idx, 11_idx}, {16_idx, 17_idx}, {27_idx, 28_idx}}));
    EXPECT_EQ(sequence2 * sequence1, IndexRangeSequence({{6_idx, 11_idx}, {16_idx, 17_idx}, {27_idx, 28_idx}}));
}

UTEST(IndexRangeSequence, IntersectSequenceNonOverlapping) {
    IndexRangeSequence sequence1({{1_idx, 11_idx}, {16_idx, 28_idx}});
    IndexRangeSequence sequence2({{29_idx, 39_idx}, {40_idx, 50_idx}});
    EXPECT_FALSE(sequence1 & sequence2);
    EXPECT_EQ(sequence1 * sequence2, IndexRangeSequence());
    EXPECT_EQ(sequence2 * sequence1, IndexRangeSequence());
}

UTEST(IndexRangeSequence, UnionOverlapping) {
    {
        IndexRangeSequence sequence({{1_idx, 11_idx}, {16_idx, 26_idx}});
        IndexRange range(6_idx, 17_idx);
        EXPECT_EQ(sequence + range, IndexRangeSequence({{1_idx, 26_idx}}));
        EXPECT_EQ(range + sequence, IndexRangeSequence({{1_idx, 26_idx}}));
    }
    {
        IndexRangeSequence sequence({{1_idx, 11_idx}, {16_idx, 26_idx}});
        IndexRange range(6_idx, 15_idx);
        EXPECT_EQ(sequence + range, IndexRangeSequence({{1_idx, 15_idx}, {16_idx, 26_idx}}));
        EXPECT_EQ(range + sequence, IndexRangeSequence({{1_idx, 15_idx}, {16_idx, 26_idx}}));
    }
}

UTEST(IndexRangeSequence, UnionNonOverlapping) {
    IndexRangeSequence sequence1({{1_idx, 11_idx}, {16_idx, 28_idx}});
    IndexRangeSequence sequence2({{29_idx, 39_idx}, {40_idx, 50_idx}});
    EXPECT_EQ(
        sequence1 + sequence2,
        IndexRangeSequence({{1_idx, 11_idx}, {16_idx, 28_idx}, {29_idx, 39_idx}, {40_idx, 50_idx}})
    );
    EXPECT_EQ(
        sequence2 + sequence1,
        IndexRangeSequence({{1_idx, 11_idx}, {16_idx, 28_idx}, {29_idx, 39_idx}, {40_idx, 50_idx}})
    );
}

UTEST(IndexSet, Empty) {
    IndexSet set;
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.size(), 0);
    EXPECT_EQ(set.range(), 0);
    EXPECT_EQ(set.density(), 0.0);
}

UTEST(IndexSet, Format) {
    IndexSet set({1_idx, 2_idx, 3_idx});
    EXPECT_EQ(fmt::format("{}", set), "[1, 2, 3]");
}

UTEST(IndexSet, SetIntersection) {
    IndexSet set1({1_idx, 2_idx, 3_idx});
    IndexSet set2({2_idx, 3_idx, 4_idx});
    EXPECT_EQ(set1 * set2, IndexSet({2_idx, 3_idx}));
    EXPECT_EQ(set2 * set1, IndexSet({2_idx, 3_idx}));
}

UTEST(IndexSet, SetUnion) {
    IndexSet set1({1_idx, 2_idx, 3_idx});
    IndexSet set2({2_idx, 3_idx, 4_idx});
    EXPECT_EQ(set1 + set2, IndexSet({1_idx, 2_idx, 3_idx, 4_idx}));
    EXPECT_EQ(set2 + set1, IndexSet({1_idx, 2_idx, 3_idx, 4_idx}));
}

UTEST(IndexSet, SetDifference) {
    IndexSet set1({1_idx, 2_idx, 3_idx});
    IndexSet set2({2_idx, 3_idx, 4_idx});
    EXPECT_EQ(set1 - set2, IndexSet({1_idx}));
    EXPECT_EQ(set2 - set1, IndexSet({4_idx}));
}

UTEST(IndexSet, SetEquality) {
    IndexSet set1({1_idx, 2_idx, 3_idx});
    IndexSet set2({1_idx, 2_idx, 3_idx});
    EXPECT_EQ(set1, set2);
}

UTEST(IndexSet, RangeIntersection) {
    IndexSet set({1_idx, 2_idx, 3_idx});
    IndexRange range(2_idx, 3_idx);
    EXPECT_EQ(set * range, IndexSet({2_idx}));
    EXPECT_EQ(range * set, IndexSet({2_idx}));
}

UTEST(IndexSet, RangeSequenceIntersection) {
    IndexSet set({1_idx, 2_idx, 3_idx});
    IndexRangeSequence sequence({{1_idx, 2_idx}, {3_idx, 4_idx}});
    EXPECT_EQ(sequence * set, IndexSet({1_idx, 3_idx}));
}

UTEST(IndexSetView, Empty) {
    IndexSetView view;
    EXPECT_TRUE(view.empty());
    EXPECT_EQ(view.size(), 0);
    EXPECT_EQ(view.range(), 0);
    EXPECT_EQ(view.density(), 0.0);
}

UTEST(IndexSetView, Format) {
    IndexSet set({1_idx, 2_idx, 3_idx});
    IndexSetView view(set);
    EXPECT_EQ(fmt::format("{}", view), "[1, 2, 3]");
}

UTEST(IndexSetView, SetIntersection) {
    IndexSet set1({1_idx, 2_idx, 3_idx});
    IndexSet set2({2_idx, 3_idx, 4_idx});
    IndexSetView view1(set1);
    IndexSetView view2(set2);
    EXPECT_EQ(view1 * view2, IndexSet({2_idx, 3_idx}));
    EXPECT_EQ(view2 * view1, IndexSet({2_idx, 3_idx}));
    EXPECT_EQ(view1 * set2, IndexSet({2_idx, 3_idx}));
    EXPECT_EQ(set1 * view2, IndexSet({2_idx, 3_idx}));
}

UTEST(IndexSetView, SetUnion) {
    IndexSet set1({1_idx, 2_idx, 3_idx});
    IndexSet set2({2_idx, 3_idx, 4_idx});
    IndexSetView view1(set1);
    IndexSetView view2(set2);
    EXPECT_EQ(view1 + view2, IndexSet({1_idx, 2_idx, 3_idx, 4_idx}));
    EXPECT_EQ(view2 + view1, IndexSet({1_idx, 2_idx, 3_idx, 4_idx}));
    EXPECT_EQ(view1 + set2, IndexSet({1_idx, 2_idx, 3_idx, 4_idx}));
    EXPECT_EQ(set1 + view2, IndexSet({1_idx, 2_idx, 3_idx, 4_idx}));
}

UTEST(IndexSetView, SetDifference) {
    IndexSet set1({1_idx, 2_idx, 3_idx});
    IndexSet set2({2_idx, 3_idx, 4_idx});
    IndexSetView view1(set1);
    IndexSetView view2(set2);
    EXPECT_EQ(view1 - view2, IndexSet({1_idx}));
    EXPECT_EQ(view2 - view1, IndexSet({4_idx}));
    EXPECT_EQ(view1 - set2, IndexSet({1_idx}));
    EXPECT_EQ(set1 - view2, IndexSet({1_idx}));
}

UTEST(IndexSetView, RangeDifference) {
    IndexRange range(1_idx, 11_idx);
    IndexSet set({1_idx, 5_idx, 8_idx});

    EXPECT_EQ(range - set, IndexRangeSequence({{2_idx, 5_idx}, {6_idx, 8_idx}, {9_idx, 11_idx}}));
    // EXPECT_EQ(sequence - set, IndexRangeSequence({{1_idx, 2_idx}, {3_idx, 4_idx}}));
}

UTEST(IndexSetView, RangeSequenceDifference) {
    IndexRangeSequence sequence({{1_idx, 5_idx}, {10_idx, 20_idx}});
    IndexSet set({1_idx, 3_idx, 8_idx, 15_idx});
    EXPECT_EQ(sequence - set, IndexRangeSequence({{2_idx, 3_idx}, {4_idx, 5_idx}, {10_idx, 15_idx}, {16_idx, 20_idx}}));
}

}  // namespace slugkit::generator::filter