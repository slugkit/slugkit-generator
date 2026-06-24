#include <slugkit/generator/filter/filtered_ids.hpp>

namespace slugkit::generator::filter {

//------------------------------------------------------------------------------------------------
// IndexRange & Sequence operations
//------------------------------------------------------------------------------------------------
auto IndexRangeSequence::operator*(const IndexRange& other) const noexcept -> IndexRangeSequence {
    std::vector<IndexRange> result;
    for (const auto& range : ranges_) {
        if (range & other) {
            result.push_back(range * other);
        }
        if (other.end() < range.front()) {
            break;
        }
    }
    return IndexRangeSequence(std::move(result));
}

auto IndexRangeSequence::operator*(const IndexRangeSequence& other) const noexcept -> IndexRangeSequence {
    std::vector<IndexRange> result;
    // we need to iterate over the ranges in the order of the ranges in lhs and rhs
    // and push the intersections to the result
    auto lhs_it = ranges_.begin();
    auto rhs_it = other.ranges_.begin();
    while (lhs_it != ranges_.end() && rhs_it != other.ranges_.end()) {
        if (*lhs_it & *rhs_it) {
            auto intersection = *lhs_it * *rhs_it;
            if (!result.empty() && (result.back() & intersection)) {
                // overlapping range with existing range in result
                result.back() = IndexRange(
                    std::min(result.back().front(), intersection.front()),
                    std::max(result.back().end(), intersection.end())
                );
            } else {
                result.push_back(intersection);
            }
        }
        // we increment the iterator of the range that is smaller
        if (*lhs_it < *rhs_it) {
            ++lhs_it;
        } else {
            ++rhs_it;
        }
    }
    return IndexRangeSequence(std::move(result));
}

/// @brief Union of two index ranges.
auto operator+(const IndexRange& lhs, const IndexRange& rhs) noexcept -> IndexRangeSequence {
    if (rhs < lhs) {
        return rhs + lhs;
    }
    if (lhs.empty()) {
        return IndexRangeSequence(rhs);
    }
    if (rhs.empty()) {
        return IndexRangeSequence(lhs);
    }
    if (lhs & rhs || lhs.end() == rhs.front()) {
        // overlapping range
        return IndexRangeSequence(std::min(lhs.front(), rhs.front()), std::max(lhs.end(), rhs.end()));
    }
    return IndexRangeSequence({lhs, rhs});
}

/// @brief Union of index range sequence and index range.
auto operator+(const IndexRangeSequence& lhs, const IndexRange& rhs) noexcept -> IndexRangeSequence {
    if (rhs.size() == 0) {
        return lhs;
    }
    if (lhs.size() == 0) {
        return IndexRangeSequence(rhs);
    }
    if (lhs.size() == 1) {
        return (*lhs.begin() + rhs);
    }
    std::vector<IndexRange> result;
    result.reserve(lhs.size());
    for (const auto& range : lhs) {
        // check intersection of back with current range
        // intersection with rhs is handled separately
        if (!result.empty() && (result.back() & range)) {
            // overlapping range
            result.back() =
                IndexRange(std::min(result.back().front(), range.front()), std::max(result.back().end(), range.end()));
        } else {
            if (range & rhs) {
                // rhs range overlaps with current range
                result.push_back(IndexRange(std::min(rhs.front(), range.front()), std::max(rhs.end(), range.end())));
                continue;
            } else if ((rhs < range) && (result.empty() || result.back().back() < rhs.front())) {
                // we push rhs as is only if it's before current range and after the last result range
                result.push_back(rhs);
            }
            result.push_back(range);
        }
    }
    return IndexRangeSequence(std::move(result));
}

/// @brief Union of two index range sequences.
auto operator+(const IndexRangeSequence& lhs, const IndexRangeSequence& rhs) noexcept -> IndexRangeSequence {
    if (lhs.empty()) {
        return rhs;
    }
    if (rhs.empty()) {
        return lhs;
    }
    if (lhs.size() == 1) {
        return rhs + *lhs.begin();
    }
    if (rhs.size() == 1) {
        return lhs + *rhs.begin();
    }
    // we need to merge the ranges in the order of the ranges in lhs and rhs
    std::vector<IndexRange> result;
    result.reserve(lhs.size() + rhs.size());
    auto lhs_it = lhs.begin();
    auto rhs_it = rhs.begin();
    while (lhs_it != lhs.end() && rhs_it != rhs.end()) {
        if (!result.empty() && (result.back() & *lhs_it)) {
            // overlapping range with existing range in result
            result.back() = IndexRange(
                std::min(result.back().front(), lhs_it->front()), std::max(result.back().end(), lhs_it->end())
            );
            ++lhs_it;
        } else if (!result.empty() && (result.back() & *rhs_it)) {
            // overlapping range with existing range in result
            result.back() = IndexRange(
                std::min(result.back().front(), rhs_it->front()), std::max(result.back().end(), rhs_it->end())
            );
        } else if (*lhs_it & *rhs_it || lhs_it->end() == rhs_it->front() || rhs_it->end() == lhs_it->front()) {
            // overlapping range
            result.push_back(
                IndexRange(std::min(lhs_it->front(), rhs_it->front()), std::max(lhs_it->end(), rhs_it->end()))
            );
            ++lhs_it;
            ++rhs_it;
        } else if (*lhs_it < *rhs_it) {
            result.push_back(*lhs_it);
            ++lhs_it;
        } else {
            result.push_back(*rhs_it);
            ++rhs_it;
        }
    }
    while (lhs_it != lhs.end()) {
        if (!result.empty() && (result.back() & *lhs_it)) {
            // overlapping range with existing range in result
            result.back() = IndexRange(
                std::min(result.back().front(), lhs_it->front()), std::max(result.back().end(), lhs_it->end())
            );
            ++lhs_it;
        } else {
            result.push_back(*lhs_it);
            ++lhs_it;
        }
    }
    while (rhs_it != rhs.end()) {
        if (!result.empty() && (result.back() & *rhs_it)) {
            // overlapping range with existing range in result
            result.back() = IndexRange(
                std::min(result.back().front(), rhs_it->front()), std::max(result.back().end(), rhs_it->end())
            );
            ++rhs_it;
        } else {
            result.push_back(*rhs_it);
            ++rhs_it;
        }
    }
    return IndexRangeSequence(std::move(result));
}

auto operator-(const IndexRange& lhs, const IndexSetView& rhs) noexcept -> IndexRangeSequence {
    if (lhs.empty()) {
        return IndexRangeSequence();
    }
    if (rhs.empty()) {
        return IndexRangeSequence(lhs);
    }
    if (!(lhs & rhs)) {  // no intersection
        return IndexRangeSequence(lhs);
    }
    std::vector<IndexRange> result;
    result.reserve(rhs.size() + 1);  // with N holes we can have at most N+1 ranges in the result
    IndexRange current_range = lhs;
    for (const auto& index : rhs) {
        if (!current_range.contains(index)) {
            continue;
        }
        auto range = IndexRange(current_range.front(), index);
        if (!range.empty()) {
            result.push_back(range);
        }
        current_range = IndexRange(index + 1, current_range.end());
    }
    result.push_back(current_range);
    return IndexRangeSequence(std::move(result));
}

auto operator-(const IndexRangeSequence& lhs, const IndexSetView& rhs) noexcept -> IndexRangeSequence {
    if (lhs.empty()) {
        return IndexRangeSequence();
    }
    if (rhs.empty()) {
        return lhs;
    }
    if (!(lhs & rhs)) {  // no intersection
        return lhs;
    }
    std::vector<IndexRange> result;
    result.reserve(rhs.size() + 1);  // with N holes we can have at most N+1 ranges in the result
    for (const auto& range : lhs) {
        auto diff = range - rhs;
        if (!diff.empty()) {
            for (const auto& sub_range : diff) {
                result.push_back(sub_range);
            }
        }
    }
    return IndexRangeSequence(std::move(result));
}

}  // namespace slugkit::generator::filter