#pragma once

#include <slugkit/utils/text.hpp>

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace slugkit::generator {

/// @brief Collision-free enumeration of the mixed-case variants of a filtered word set.
///
/// A mixed-case selector multiplies a word's capacity by the number of distinct cased forms it
/// has: a word with @c k letters yields @c 2^k forms. To keep generation collision-free, each
/// word @c i occupies a contiguous block of @c 2^toggleable(word_i) sequence slots; the total
/// over all words is the selector's true capacity. A permuted sequence value decomposes into a
/// @c (word index, compact case index) pair, where the compact index drives
/// utils::text::ExpandCaseMask. This makes the reported capacity equal to the exact number of
/// distinct mixed-case strings (no duplicates, unlike a uniform 2^max_length mask).
///
/// The same block layout is built by both the in-memory and binary dictionaries (words visited
/// in identical lexicographic order), so the two paths stay byte-identical.
class MixedCaseIndex {
public:
    MixedCaseIndex() = default;

    /// @brief Reserve storage and reset before streaming words with AddWord(). Use this when
    /// words are only available in iteration order (the binary path's index sequence).
    void Reserve(std::size_t count) {
        prefix_.clear();
        prefix_.reserve(count);
        total_ = 0;
    }

    /// @brief Append the next word's block. Words must be added in generation (lexicographic)
    /// order; @p word is the lower-case form.
    void AddWord(std::string_view word) {
        prefix_.push_back(total_);
        total_ += BlockSize(word);
    }

    /// @brief Append a word that has exactly one cased form (block size 1) -- a verbatim word with
    /// no case variants. Keeps mixed-case selection collision-free for such words (one slot that
    /// maps back to the word itself, with no case toggling applied).
    void AddUnitWord() {
        prefix_.push_back(total_);
        total_ += 1;
    }

    /// @brief Build the layout from @p count words, where @c at(i) returns the lower-case bytes
    /// of word @c i in generation order. Convenience for random-access (in-memory) dictionaries.
    template <typename WordAt>
    void Build(std::size_t count, WordAt&& at) {
        Reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            AddWord(at(i));
        }
    }

    /// @brief The selector's mixed-case capacity (sum of every word's block size).
    [[nodiscard]] auto Capacity() const noexcept -> std::uint64_t {
        return total_;
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return total_ == 0;
    }

    /// @brief Map a value in [0, Capacity()) to a (word index, compact case index) pair. The
    /// compact case index is in [0, 2^toggleable(word)) and feeds utils::text::ExpandCaseMask.
    [[nodiscard]] auto Decompose(std::uint64_t value) const -> std::pair<std::size_t, std::uint64_t> {
        // First block whose start is greater than `value`; the owning word is the one before it.
        auto it = std::upper_bound(prefix_.begin(), prefix_.end(), value);
        auto idx = static_cast<std::size_t>(it - prefix_.begin()) - 1;
        return {idx, value - prefix_[idx]};
    }

    /// @brief Number of cased forms of @p word, i.e. its block size. Capped so totals over even
    /// the largest dictionaries stay well within an int64; real slug words never approach the cap.
    [[nodiscard]] static auto BlockSize(std::string_view word) noexcept -> std::uint64_t {
        auto bits = utils::text::CountCaseToggleable(word);
        if (bits > kMaxCaseBits) {
            bits = kMaxCaseBits;
        }
        return std::uint64_t{1} << bits;
    }

private:
    // 2^56 per word leaves room for ~2^7 words before an int64 overflow could matter, far beyond
    // any realistic mixed-case word length. Surplus high bits of the case index are dropped by
    // ExpandCaseMask, so capping stays collision-free (it only limits reach, not uniqueness).
    static constexpr std::size_t kMaxCaseBits = 56;

    // prefix_[i] is the first sequence slot owned by word i; total_ is the slot count overall.
    std::vector<std::uint64_t> prefix_;
    std::uint64_t total_ = 0;
};

}  // namespace slugkit::generator
