#pragma once

#include <slugkit/generator/types.hpp>

#include <algorithm>
#include <concepts>
#include <numeric>
#include <stdexcept>
#include <variant>
#include <vector>

#include <fmt/format.h>

namespace slugkit::generator::filter {

template <typename T>
concept IndexContainer = requires(T t) {
    { t.size() } -> std::convertible_to<std::size_t>;
    { t.empty() } -> std::convertible_to<bool>;
    { t.range() } -> std::convertible_to<IndexType>;
    { t.density() } -> std::convertible_to<float>;
    { t.front() } -> std::convertible_to<IndexType>;
    { t.back() } -> std::convertible_to<IndexType>;
    { t.at(IndexType(0)) } -> std::convertible_to<IndexType>;
    { t.at(IndexType::UnderlyingType(0)) } -> std::convertible_to<IndexType>;
};

/// @brief Intersection test of two simple index containers.
template <IndexContainer Lhs, IndexContainer Rhs>
auto operator&(Lhs&& lhs, Rhs&& rhs) -> bool {
    return ((lhs.front() <= rhs.front()) && (rhs.front() <= lhs.back())) ||
           ((rhs.front() <= lhs.front()) && (lhs.front() <= rhs.back()));
}

/// @brief A range of indexes.
class IndexRange {
public:
    using vector_size_type = std::size_t;

public:
    IndexRange() noexcept = default;
    IndexRange(IndexType begin, IndexType end)
        : begin_(begin)
        , end_(end) {
        if (begin_ > end_) {
            throw std::invalid_argument(fmt::format("begin must be less or equal to end: {} > {}", begin_, end_));
        }
    }

    IndexRange(IndexRange&& other) noexcept = default;
    IndexRange(const IndexRange& other) noexcept = default;
    ~IndexRange() noexcept = default;

    auto operator=(IndexRange&& other) noexcept -> IndexRange& = default;
    auto operator=(const IndexRange& other) noexcept -> IndexRange& = default;

    auto operator==(const IndexRange& other) const noexcept -> bool = default;
    auto operator!=(const IndexRange& other) const noexcept -> bool = default;
    auto operator<(const IndexRange& other) const noexcept -> bool {
        return front() < other.front();
    }

    /// @brief The number of meaniningful indexes stored in the range.
    [[nodiscard]] constexpr auto size() const noexcept -> vector_size_type {
        if (empty()) {
            return 0;
        }
        return 1;
    }

    [[nodiscard]] constexpr auto empty() const noexcept -> bool {
        return end_ == begin_;
    }

    explicit operator bool() const noexcept {
        return !empty();
    }

    /// @brief Distance between the first and the last meaningful index in the range.
    [[nodiscard]] constexpr auto range() const noexcept -> IndexType {
        return end_ - begin_;
    }

    /// @brief The number of meaningful indexes in the range.
    [[nodiscard]] constexpr auto count() const noexcept -> IndexType {
        return end_ - begin_;
    }

    [[nodiscard]] constexpr auto density() const -> float {
        auto range = this->range().GetUnderlying();
        if (range == 0) {
            return 0.0;
        }
        return static_cast<float>(size()) / static_cast<float>(range);
    }

    auto operator[](IndexType index) const noexcept -> IndexType {
        return begin_ + index;
    }
    auto operator[](IndexType::UnderlyingType index) const noexcept -> IndexType {
        return begin_ + index;
    }

    auto front() const noexcept -> IndexType {
        return begin_;
    }
    auto back() const noexcept -> IndexType {
        return end_ - 1;
    }

    auto end() const noexcept -> IndexType {
        return end_;
    }

    auto at(IndexType index) const -> IndexType {
        if (end_ <= begin_ + index) {
            throw std::out_of_range(fmt::format("index {} is out of range: [0, {})", index.GetUnderlying(), size()));
        }
        return begin_ + index;
    }
    auto at(IndexType::UnderlyingType index) const -> IndexType {
        return at(IndexType(index));
    }

    [[nodiscard]] constexpr auto contains(IndexType index) const noexcept -> bool {
        return begin_ <= index && index < end_;
    }

    /// @brief Visit every logical index in the range in ascending order. O(count), no per-index
    /// position lookup (unlike at()).
    template <typename F>
    void for_each(F&& f) const {
        for (auto i = begin_.GetUnderlying(); i < end_.GetUnderlying(); ++i) {
            f(IndexType(i));
        }
    }

    /// @brief Intersection of two index ranges.
    auto operator*(const IndexRange& other) const noexcept -> IndexRange {
        if (!(*this & other)) {
            return IndexRange();  // empty range
        }
        return IndexRange(std::max(front(), other.front()), std::min(back(), other.back()) + 1);
    }

private:
    IndexType begin_{0};
    IndexType end_{0};
};

/// @brief A sequence of index ranges.
/// @note The ranges are sorted in ascending order and non-overlapping.
class IndexRangeSequence {
public:
    using vector_size_type = std::size_t;
    using range_container_type = std::vector<IndexRange>;
    using range_iterator_type = range_container_type::const_iterator;

public:
    IndexRangeSequence() noexcept = default;
    IndexRangeSequence(std::initializer_list<IndexRange> ranges)
        : ranges_(ranges) {
    }

    explicit IndexRangeSequence(std::vector<IndexRange> ranges)
        : ranges_(std::move(ranges)) {
    }

    explicit IndexRangeSequence(IndexRange range)
        : ranges_({std::move(range)}) {
    }

    IndexRangeSequence(IndexType begin, IndexType end)
        : ranges_({IndexRange(begin, end)}) {
    }

    IndexRangeSequence(IndexRangeSequence&& other) noexcept = default;
    IndexRangeSequence(const IndexRangeSequence& other) noexcept = default;
    ~IndexRangeSequence() noexcept = default;

    auto operator=(IndexRangeSequence&& other) noexcept -> IndexRangeSequence& = default;
    auto operator=(const IndexRangeSequence& other) noexcept -> IndexRangeSequence& = default;

    auto operator==(const IndexRangeSequence& other) const noexcept -> bool = default;
    auto operator!=(const IndexRangeSequence& other) const noexcept -> bool = default;

    [[nodiscard]] auto size() const noexcept -> vector_size_type {
        return ranges_.size();
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return ranges_.empty();
    }

    auto range() const noexcept -> IndexType {
        using namespace literals;
        if (empty()) {
            return 0_idx;
        }
        return ranges_.back().back() - ranges_.front().front();
    }

    [[nodiscard]] auto count() const noexcept -> IndexType {
        using namespace literals;
        return std::accumulate(ranges_.begin(), ranges_.end(), 0_idx, [](IndexType acc, const IndexRange& range) {
            return acc + range.count();
        });
    }

    auto density() const -> float {
        auto range = this->range().GetUnderlying();
        if (range == 0) {
            return 0.0;
        }
        return static_cast<float>(size()) / static_cast<float>(range);
    }

    auto at(IndexType index) const -> IndexType {
        using namespace literals;
        auto base_index = 0_idx;
        for (const auto& range : ranges_) {
            if (index - base_index < range.count()) {
                return range[index - base_index];
            }
            base_index = base_index + range.count();
        }
        throw std::out_of_range(fmt::format("index {} is out of range: [0, {})", index.GetUnderlying(), count()));
    }

    auto at(IndexType::UnderlyingType index) const -> IndexType {
        return at(IndexType(index));
    }

    auto front() const noexcept -> IndexType {
        return ranges_.front().front();
    }
    auto back() const noexcept -> IndexType {
        return ranges_.back().back();
    }

    //{@
    /// @name Range iteration
    /// @brief Visit every logical index across all ranges in ascending order. O(count) total,
    /// without the per-index O(ranges) at() lookup.
    template <typename F>
    void for_each(F&& f) const {
        for (const auto& range : ranges_) {
            range.for_each(f);
        }
    }

    auto begin() const noexcept -> range_iterator_type {
        return ranges_.begin();
    }
    auto end() const noexcept -> range_iterator_type {
        return ranges_.end();
    }
    //@}

    /// @brief Intersection test of index range sequence and index range.
    auto operator&(const IndexRange& other) const noexcept -> bool {
        for (const auto& range : ranges_) {
            if (range & other) {
                return true;
            }
        }
        return false;
    }

    /// @brief Intersection test of two index range sequences.
    auto operator&(const IndexRangeSequence& other) const noexcept -> bool {
        auto lhs_begin = ranges_.begin();
        auto lhs_end = ranges_.end();
        auto rhs_begin = other.ranges_.begin();
        auto rhs_end = other.ranges_.end();
        while (lhs_begin != lhs_end && rhs_begin != rhs_end) {
            if (*lhs_begin & *rhs_begin) {
                return true;
            }
            if (*lhs_begin < *rhs_begin) {
                ++lhs_begin;
            } else {
                ++rhs_begin;
            }
        }
        return false;
    }

    /// @brief Intersection of index range sequence and index range.
    auto operator*(const IndexRange& other) const noexcept -> IndexRangeSequence;
    /// @brief Intersection of two index range sequences.
    auto operator*(const IndexRangeSequence& other) const noexcept -> IndexRangeSequence;

private:
    std::vector<IndexRange> ranges_;
};

/// @brief Intersection test of index range and index range sequence.
inline auto operator&(const IndexRange& lhs, const IndexRangeSequence& rhs) noexcept -> bool {
    return rhs & lhs;
}
/// @brief Intersection of index range and index range sequence.
inline auto operator*(const IndexRange& lhs, const IndexRangeSequence& rhs) noexcept -> IndexRangeSequence {
    return rhs * lhs;
}

/// @brief Union of two index ranges.
auto operator+(const IndexRange& lhs, const IndexRange& rhs) noexcept -> IndexRangeSequence;
/// @brief Union of index range sequence and index range.
auto operator+(const IndexRangeSequence& lhs, const IndexRange& rhs) noexcept -> IndexRangeSequence;
inline auto operator+(const IndexRange& lhs, const IndexRangeSequence& rhs) noexcept -> IndexRangeSequence {
    return rhs + lhs;
}
/// @brief Union of two index range sequences.
auto operator+(const IndexRangeSequence& lhs, const IndexRangeSequence& rhs) noexcept -> IndexRangeSequence;

/// @brief A set of indexes.
/// @note The indexes are sorted in ascending order and unique.
class IndexSet {
public:
    using vector_size_type = std::size_t;
    using index_iterator_type = std::vector<IndexType>::const_iterator;

public:
    IndexSet() noexcept = default;
    IndexSet(std::initializer_list<IndexType> indexes)
        : indexes_(indexes) {
        std::sort(indexes_.begin(), indexes_.end());
        indexes_.erase(std::unique(indexes_.begin(), indexes_.end()), indexes_.end());
    }

    /// @brief Construct a set of indexes from a range of indexes.
    /// @param indexes The indexes to construct the set from.
    /// @pre The indexes are sorted and unique.
    explicit IndexSet(std::vector<IndexType> indexes)
        : indexes_(std::move(indexes)) {
    }

    /// @brief Construct a set of indexes from a range of indexes.
    /// @param begin The beginning of the range (inclusive).
    /// @param end The end of the range (exclusive).
    /// @pre The indexes are sorted and unique.
    IndexSet(const IndexType* begin, const IndexType* end)
        : indexes_(begin, end) {
    }

    /// @brief Construct a set of indexes from a range of indexes.
    /// @param begin The beginning of the range (inclusive).
    /// @param size The size of the range.
    /// @pre The indexes are sorted and unique.
    IndexSet(const IndexType* begin, const SizeType size)
        : indexes_(begin, begin + size.GetUnderlying()) {
    }

    /// @brief Construct a set of indexes from a range of indexes.
    /// @param begin The beginning of the range (inclusive).
    /// @param size The size of the range.
    /// @pre The indexes are sorted and unique.
    IndexSet(const IndexType* begin, const vector_size_type size)
        : indexes_(begin, begin + size) {
    }

    IndexSet(IndexSet&& other) noexcept = default;
    IndexSet(const IndexSet& other) noexcept = default;
    ~IndexSet() noexcept = default;

    auto operator=(IndexSet&& other) noexcept -> IndexSet& = default;
    auto operator=(const IndexSet& other) noexcept -> IndexSet& = default;

    auto operator==(const IndexSet& other) const noexcept -> bool = default;
    auto operator!=(const IndexSet& other) const noexcept -> bool = default;

    [[nodiscard]] auto size() const noexcept -> vector_size_type {
        return indexes_.size();
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return indexes_.empty();
    }

    [[nodiscard]] auto range() const noexcept -> IndexType {
        using namespace literals;
        if (empty()) {
            return 0_idx;
        }
        return back() - front() + 1_idx;
    }

    [[nodiscard]] auto count() const noexcept -> IndexType {
        return IndexType(size());
    }

    /// @brief Visit every index in the set in ascending order.
    template <typename F>
    void for_each(F&& f) const {
        for (const auto& index : indexes_) {
            f(index);
        }
    }

    [[nodiscard]] auto density() const -> float {
        auto range = this->range().GetUnderlying();
        if (range == 0) {
            return 0.0;
        }
        return static_cast<float>(size()) / static_cast<float>(range);
    }

    auto operator[](IndexType index) const noexcept -> IndexType {
        return indexes_[index.GetUnderlying()];
    }
    auto operator[](IndexType::UnderlyingType index) const noexcept -> IndexType {
        return indexes_[index];
    }

    auto front() const noexcept -> IndexType {
        return indexes_.front();
    }
    auto back() const noexcept -> IndexType {
        return indexes_.back();
    }

    auto at(IndexType index) const -> IndexType {
        if (index.GetUnderlying() >= indexes_.size()) {
            throw std::out_of_range(fmt::format("index {} is out of range: [0, {})", index.GetUnderlying(), size()));
        }
        return indexes_[index.GetUnderlying()];
    }

    auto at(IndexType::UnderlyingType index) const -> IndexType {
        return at(IndexType(index));
    }

    /// @brief Intersection of two index sets.
    auto operator*(const IndexSet& other) const noexcept -> IndexSet {
        if (!(*this & other)) {
            return IndexSet();  // empty set
        }
        std::vector<IndexType> result;
        std::set_intersection(
            indexes_.begin(), indexes_.end(), other.indexes_.begin(), other.indexes_.end(), std::back_inserter(result)
        );
        return IndexSet(std::move(result));
    }

    /// @brief Union of two index sets.
    auto operator+(const IndexSet& other) const noexcept -> IndexSet {
        std::vector<IndexType> result;
        std::set_union(
            indexes_.begin(), indexes_.end(), other.indexes_.begin(), other.indexes_.end(), std::back_inserter(result)
        );
        return IndexSet(std::move(result));
    }

    /// @brief Difference of two index sets.
    /// @note The result is the set of indexes that are in the first set but not in the second.
    auto operator-(const IndexSet& other) const noexcept -> IndexSet {
        std::vector<IndexType> result;
        std::set_difference(
            indexes_.begin(), indexes_.end(), other.indexes_.begin(), other.indexes_.end(), std::back_inserter(result)
        );
        return IndexSet(std::move(result));
    }

    //@{
    /// @name Set iteration
    auto begin() const noexcept -> index_iterator_type {
        return indexes_.begin();
    }
    auto end() const noexcept -> index_iterator_type {
        return indexes_.end();
    }
    //@}

private:
    friend auto operator*(const IndexSet& lhs, const IndexRange& rhs) noexcept -> IndexSet;
    friend auto operator*(const IndexSet& lhs, const IndexRangeSequence& rhs) noexcept -> IndexSet;

    std::vector<IndexType> indexes_;
};

/// @brief Intersection test of index set and index range sequence.
inline auto operator&(const IndexSet& lhs, const IndexRangeSequence& rhs) noexcept -> bool {
    for (const auto& range : rhs) {
        if (lhs & range) {
            return true;
        }
    }
    return false;
}

/// @brief Intersection of index set and index range.
inline auto operator*(const IndexSet& lhs, const IndexRange& rhs) noexcept -> IndexSet {
    if (!(lhs & rhs)) {
        return IndexSet();  // empty set
    }
    auto begin = std::lower_bound(lhs.indexes_.begin(), lhs.indexes_.end(), rhs.front());
    auto end = std::upper_bound(lhs.indexes_.begin(), lhs.indexes_.end(), rhs.back());
    return IndexSet(&*begin, &*end);
}

inline auto operator*(const IndexRange& lhs, const IndexSet& rhs) noexcept -> IndexSet {
    return rhs * lhs;
}

/// @brief Intersection of index set and index range sequence.
inline auto operator*(const IndexSet& lhs, const IndexRangeSequence& rhs) noexcept -> IndexSet {
    std::vector<IndexType> result;
    result.reserve(lhs.size());
    for (const auto& range : rhs) {
        auto begin = std::lower_bound(lhs.indexes_.begin(), lhs.indexes_.end(), range.front());
        auto end = std::upper_bound(lhs.indexes_.begin(), lhs.indexes_.end(), range.back());
        result.insert(result.end(), begin, end);
    }
    return IndexSet(std::move(result));
}

inline auto operator*(const IndexRangeSequence& lhs, const IndexSet& rhs) noexcept -> IndexSet {
    return rhs * lhs;
}

/// @brief A view of an index set.
/// @note The view is a pair of pointers to the first index in the set and the one after the last index.
/// The view can be constructed from SparseIndex or IndexSet. It helps avoiding unnecessary copies of the index set.
class IndexSetView final {
public:
    using vector_size_type = std::size_t;
    using iterator_type = const IndexType*;

public:
    IndexSetView() noexcept = default;
    IndexSetView(IndexSetView&& set) noexcept = default;
    IndexSetView(const IndexSetView& set) noexcept = default;

    IndexSetView(iterator_type begin, iterator_type end) noexcept
        : begin_(begin)
        , end_(end) {
    }

    /* implicit */ IndexSetView(const IndexSet& set) noexcept
        : begin_(&*set.begin())
        , end_(&*set.end()) {
    }

    auto operator=(IndexSetView&& other) noexcept -> IndexSetView& = default;
    auto operator=(const IndexSetView& other) noexcept -> IndexSetView& = default;

    auto operator==(const IndexSetView& other) const noexcept -> bool = default;
    auto operator!=(const IndexSetView& other) const noexcept -> bool = default;

    [[nodiscard]] auto size() const noexcept -> vector_size_type {
        return end_ - begin_;
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return begin_ == end_;
    }

    [[nodiscard]] auto range() const noexcept -> IndexType {
        return IndexType(end_ - begin_);
    }

    [[nodiscard]] auto count() const noexcept -> IndexType {
        return IndexType(end_ - begin_);
    }

    [[nodiscard]] auto density() const -> float {
        auto range = this->range().GetUnderlying();
        if (range == 0) {
            return 0.0;
        }
        return static_cast<float>(size()) / static_cast<float>(range);
    }

    auto at(IndexType index) const -> IndexType {
        return begin_[index.GetUnderlying()];
    }

    /// @brief Visit every index in the view in ascending order.
    template <typename F>
    void for_each(F&& f) const {
        for (auto it = begin_; it != end_; ++it) {
            f(*it);
        }
    }

    auto at(IndexType::UnderlyingType index) const -> IndexType {
        return begin_[index];
    }

    auto front() const noexcept -> IndexType {
        return *begin_;
    }

    auto back() const noexcept -> IndexType {
        return *(end_ - 1);
    }

    auto begin() const noexcept -> iterator_type {
        return begin_;
    }

    auto end() const noexcept -> iterator_type {
        return end_;
    }

    /// @brief Intersection of index set view and index range.
    /// @note Cutting the view by an index range.
    /// can avoid materializing the result.
    auto operator*(const IndexRange& other) const noexcept -> IndexSetView {
        return IndexSetView(
            std::lower_bound(begin_, end_, other.front()), std::upper_bound(begin_, end_, other.back())
        );
    }

    /// @brief Intersection of index set view and index range sequence.
    /// @note Cutting the view by an index range sequence.
    /// can not avoid materializing the result.
    auto operator*(const IndexRangeSequence& other) const noexcept -> IndexSet {
        std::vector<IndexType> result;
        result.reserve(size());
        for (const auto& range : other) {
            auto begin = std::lower_bound(begin_, end_, range.front());
            auto end = std::upper_bound(begin_, end_, range.back());
            result.insert(result.end(), begin, end);
        }
        return IndexSet(std::move(result));
    }

    /// @brief Intersection of two index sets.
    /// A proper set intersection.
    /// can not avoid materializing the result.
    auto operator*(const IndexSetView& other) const noexcept -> IndexSet {
        if (empty() || other.empty() || !(*this & other)) {
            return IndexSet();  // empty set
        }
        std::vector<IndexType> result;
        std::set_intersection(begin_, end_, other.begin_, other.end_, std::back_inserter(result));
        return IndexSet(std::move(result));
    }

    /// @brief Union of two index sets.
    /// can not avoid materializing the result.
    auto operator+(const IndexSetView& other) const noexcept -> IndexSet {
        std::vector<IndexType> result;
        std::set_union(begin_, end_, other.begin_, other.end_, std::back_inserter(result));
        return IndexSet(std::move(result));
    }

    /// @brief Difference of two index sets.
    /// @note The result is the set of indexes that are in the first set but not in the second.
    /// can not avoid materializing the result.
    auto operator-(const IndexSetView& other) const noexcept -> IndexSet {
        std::vector<IndexType> result;
        std::set_difference(begin_, end_, other.begin_, other.end_, std::back_inserter(result));
        return IndexSet(std::move(result));
    }

private:
    iterator_type begin_{nullptr};
    iterator_type end_{nullptr};
};

static_assert(IndexContainer<IndexSetView>);

// TODO make the ops template functions with concepts
inline auto operator*(const IndexRange& lhs, const IndexSetView& rhs) noexcept -> IndexSetView {
    return rhs * lhs;
}

inline auto operator*(const IndexRangeSequence& lhs, const IndexSetView& rhs) noexcept -> IndexSet {
    return rhs * lhs;
}

inline auto operator*(const IndexSet& lhs, const IndexSetView& rhs) noexcept -> IndexSet {
    return rhs * lhs;
}

inline auto operator+(const IndexSet& lhs, const IndexSetView& rhs) noexcept -> IndexSet {
    return rhs + lhs;
}

inline auto operator-(const IndexSet& lhs, const IndexSetView& rhs) noexcept -> IndexSet {
    return IndexSetView(lhs) - rhs;
}

// will make 'holes' in the result
auto operator-(const IndexRange& lhs, const IndexSetView& rhs) noexcept -> IndexRangeSequence;
auto operator-(const IndexRangeSequence& lhs, const IndexSetView& rhs) noexcept -> IndexRangeSequence;

using IndexesChunk = std::variant<IndexRangeSequence, IndexSet, IndexSetView>;

class IndexSequence {
public:
    using vector_size_type = std::size_t;

public:
    IndexSequence() noexcept = default;

    template <IndexContainer Container>
    requires std::is_constructible_v<IndexesChunk, Container>
    /* implicit */ IndexSequence(Container&& container) noexcept
        : chunk_(std::forward<Container>(container)) {
    }

    IndexSequence(IndexSequence&& other) noexcept = default;
    IndexSequence(const IndexSequence& other) noexcept = default;
    ~IndexSequence() noexcept = default;

    auto operator=(IndexSequence&& other) noexcept -> IndexSequence& = default;
    auto operator=(const IndexSequence& other) noexcept -> IndexSequence& = default;

    [[nodiscard]] auto size() const noexcept -> vector_size_type {
        return std::visit([](const auto& chunk) { return chunk.size(); }, chunk_);
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return std::visit([](const auto& chunk) { return chunk.empty(); }, chunk_);
    }

    [[nodiscard]] auto range() const noexcept -> IndexType {
        return std::visit([](const auto& chunk) { return chunk.range(); }, chunk_);
    }

    [[nodiscard]] auto count() const noexcept -> IndexType {
        return std::visit([](const auto& chunk) { return chunk.count(); }, chunk_);
    }

    [[nodiscard]] auto density() const -> float {
        return std::visit([](const auto& chunk) { return chunk.density(); }, chunk_);
    }

    auto at(IndexType index) const -> IndexType {
        return std::visit([index](const auto& chunk) { return chunk.at(index); }, chunk_);
    }

    auto at(IndexType::UnderlyingType index) const -> IndexType {
        return at(IndexType(index));
    }

    /// @brief Visit every logical index in ascending order in O(count) (no per-index at()).
    template <typename F>
    void for_each(F&& f) const {
        std::visit([&f](const auto& chunk) { chunk.for_each(f); }, chunk_);
    }

    auto front() const noexcept -> IndexType {
        return std::visit([](const auto& chunk) { return chunk.front(); }, chunk_);
    }

    auto back() const noexcept -> IndexType {
        return std::visit([](const auto& chunk) { return chunk.back(); }, chunk_);
    }

    template <IndexContainer Other>
    auto operator*(const Other& other) const noexcept -> IndexSequence {
        return std::visit([other](const auto& chunk) { return IndexSequence(chunk * other); }, chunk_);
    }

    template <IndexContainer Other>
    auto operator+(const Other& other) const noexcept -> IndexSequence {
        return std::visit([other](const auto& chunk) { return IndexSequence(chunk + other); }, chunk_);
    }

    template <IndexContainer Other>
    auto operator-(const Other& other) const noexcept -> IndexSequence {
        return std::visit([other](const auto& chunk) { return IndexSequence(chunk - other); }, chunk_);
    }

private:
    IndexesChunk chunk_{};
};

}  // namespace slugkit::generator::filter