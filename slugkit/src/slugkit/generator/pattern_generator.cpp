#include <slugkit/generator/pattern_generator.hpp>

#include <slugkit/generator/constants.hpp>
#include <slugkit/generator/exceptions.hpp>
#include <slugkit/generator/permutations.hpp>
#include <slugkit/utils/primes.hpp>
#include <slugkit/utils/roman.hpp>
#include <slugkit/utils/text.hpp>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <unordered_set>

namespace slugkit::generator {

namespace {

using RomanNumerals = std::array<std::string, 3999>;
using FilteredRomanNumerals = std::vector<RomanNumerals::const_iterator>;

constexpr std::uint32_t kSeedStep = 2083;  // just a random prime number

}  // namespace

struct FilteredRomanDictionary {
    FilteredRomanDictionary(const RomanNumerals& roman_dictionary, const std::size_t max_length)
        : max_length{max_length} {
        for (auto it = roman_dictionary.begin(); it != roman_dictionary.end(); ++it) {
            if (it->size() <= max_length) {
                filtered_numerals.push_back(it);
            }
        }
    }

    std::size_t GetCapacity() const {
        return filtered_numerals.size();
    }

    std::size_t max_length;
    FilteredRomanNumerals filtered_numerals;
};

namespace {
// 32 special symbols, so we can use hex numbers
// to get stable permutations
constexpr auto kSpecialSymbols = "!@#$%^&*()_+-=[]{}|;:,.<>?'\"~/\\?";
constexpr auto kSpecialSymbolsCount = 32;

struct RomanDictionary {
    RomanDictionary() {
        for (int i = 1; i <= 3999; ++i) {
            numerals[i - 1] = utils::roman::ToRoman(i);
        }
    }

    RomanNumerals numerals;

    FilteredRomanDictionaryPtr Filter(const NumberGen& selector) const {
        return std::make_shared<FilteredRomanDictionary>(numerals, selector.max_length);
    }
};

const RomanDictionary kRomanDictionary;

}  // namespace

//-------------------------------------------------------------
// SelectorSubstitutionGenerator
//-------------------------------------------------------------
SelectorSubstitutionGenerator::SelectorSubstitutionGenerator(
    FilteredDictionaryConstPtr dictionary,
    const SelectorSettings& settings
)
    : dictionary_{dictionary}
    , selected_size_{settings.selected_size} {
}

std::string SelectorSubstitutionGenerator::Generate(std::uint32_t seed, std::size_t sequence_number) const {
    auto index = Permute(selected_size_, seed, sequence_number);
    if (dictionary_->GetCase() == CaseType::kMixed) {
        // For mixed case selected_size_ is the mixed-case capacity (sum of every word's case
        // space). Decompose the permuted value into a word and its case mask so each sequence
        // value maps to a distinct cased slug -- collision-free, unlike a uniform per-word mask.
        auto [word_index, case_index] = dictionary_->DecomposeMixed(static_cast<std::uint64_t>(index));
        const auto& word = dictionary_->GetWord(word_index).word;
        auto mask = utils::text::ExpandCaseMask(word, case_index);
        return utils::text::MixedCase(word, utils::text::kEnUsLocale, mask);
    }
    return (*dictionary_)[index];
}

std::size_t SelectorSubstitutionGenerator::GetMaxLength() const {
    return dictionary_->GetMaxLength();
}

//-------------------------------------------------------------
// BinarySelectorSubstitutionGenerator
//-------------------------------------------------------------
BinarySelectorSubstitutionGenerator::BinarySelectorSubstitutionGenerator(
    binary::FilteredDictionaryConstPtr dictionary,
    const SelectorSettings& settings
)
    : dictionary_{std::move(dictionary)}
    , selected_size_{settings.selected_size} {
}

std::string BinarySelectorSubstitutionGenerator::Generate(std::uint32_t seed, std::size_t sequence_number) const {
    auto index = Permute(selected_size_, seed, sequence_number);
    // The dictionary stores precomputed case variants, so pick the matching one instead of
    // converting at generation time. kMixed decomposes the permuted value into a word and its
    // case mask (selected_size_ is the mixed-case capacity), then applies the lowercase variant.
    if (dictionary_->GetCase() == CaseType::kMixed) {
        auto [word_index, case_index] = dictionary_->DecomposeMixed(static_cast<std::uint64_t>(index));
        const auto& entry = (*dictionary_)[IndexType(static_cast<IndexType::UnderlyingType>(word_index))];
        // Verbatim words have a single form (unit block, case_index == 0): emit the original
        // text without applying a case mask, which would otherwise re-case the fixed phrase.
        if (entry.IsVerbatim()) {
            return std::string{entry.Lowercase()};
        }
        auto word = entry.Lowercase();
        auto mask = utils::text::ExpandCaseMask(word, case_index);
        return utils::text::MixedCase(word, utils::text::kEnUsLocale, mask);
    }
    const auto& entry = (*dictionary_)[IndexType(static_cast<IndexType::UnderlyingType>(index))];
    // The upper/title variants are empty for a verbatim word (and, more generally, for any word
    // compiled without that case). Fall back to the base slot, which holds the word as written.
    switch (dictionary_->GetCase()) {
        case CaseType::kUpper: {
            auto uppercase = entry.Uppercase();
            return std::string{uppercase.empty() ? entry.Lowercase() : uppercase};
        }
        case CaseType::kTitle: {
            auto titlecase = entry.Titlecase();
            return std::string{titlecase.empty() ? entry.Lowercase() : titlecase};
        }
        case CaseType::kNone:
        case CaseType::kLower:
        case CaseType::kMixed:  // handled above
        default:
            return std::string{entry.Lowercase()};
    }
}

std::size_t BinarySelectorSubstitutionGenerator::GetMaxLength() const {
    return dictionary_->GetMaxLength();
}

//-------------------------------------------------------------
// NumberSubstitutionGenerator
//-------------------------------------------------------------
NumberSubstitutionGenerator::NumberSubstitutionGenerator(const NumberGen& number_gen)
    : base_{number_gen.base}
    , max_length_{static_cast<std::uint32_t>(number_gen.max_length)} {
    if (base_ == NumberBase::kRoman) {
        throw std::runtime_error("Roman numbers are supposed to be substituted by a separate generator");
    }
    if (base_ == NumberBase::kDec && max_length_ > constants::kMaxDecimalLength) {
        throw std::runtime_error("Decimal number length is too long");
    } else if ((base_ == NumberBase::kHex || base_ == NumberBase::kHexUpper) &&
               max_length_ > constants::kMaxHexLength) {
        throw std::runtime_error("Hex number length is too long");
    }
}

std::string NumberSubstitutionGenerator::Generate(std::uint32_t seed, std::size_t sequence_number) const {
    std::uint64_t max_value = 0;
    switch (base_) {
        case NumberBase::kDec:
            max_value = static_cast<std::uint64_t>(std::pow(10, max_length_));
            break;
        case NumberBase::kHex:
        case NumberBase::kHexUpper:
            if (max_length_ < constants::kMaxHexLength) {
                max_value = 1ULL << (max_length_ * 4);
            }
            break;
        case NumberBase::kRoman:
        case NumberBase::kRomanLower:
            throw std::runtime_error("Roman numbers are supposed to be substituted by a separate generator");
            break;
    }
    // Permute dispatches correctly to PermutePowerOf2 or Permute
    auto value = Permute(max_value, seed, sequence_number);
    if (base_ == NumberBase::kDec) {
        return fmt::format("{:0{}d}", value, max_length_);
    }
    if (base_ == NumberBase::kHex) {
        return fmt::format("{:0{}x}", value, max_length_);
    }
    return fmt::format("{:0{}X}", value, max_length_);
}

numeric::BigInt NumberSubstitutionGenerator::GetCapacity() const {
    switch (base_) {
        case NumberBase::kDec:
            return numeric::BigInt(static_cast<std::size_t>(std::pow(10, max_length_)));
        case NumberBase::kHex:
        case NumberBase::kHexUpper:
            return numeric::BigInt{1ULL} << (max_length_ * 4);
        default:
            throw std::runtime_error("Roman numbers are supposed to be substituted by a separate generator");
    }
}

//-------------------------------------------------------------
// RomanSubstitutionGenerator
//-------------------------------------------------------------
RomanSubstitutionGenerator::RomanSubstitutionGenerator(const NumberGen& number_gen)
    : roman_dictionary_{kRomanDictionary.Filter(number_gen)}
    , base_{number_gen.base}
    , max_length_{static_cast<std::uint32_t>(number_gen.max_length)} {
}

std::string RomanSubstitutionGenerator::Generate(std::uint32_t seed, std::size_t sequence_number) const {
    auto index = Permute(roman_dictionary_->GetCapacity(), seed, sequence_number);
    auto value = roman_dictionary_->filtered_numerals[index];
    if (base_ == NumberBase::kRomanLower) {
        return utils::text::ToLower(*value, utils::text::kEnUsLocale);
    }
    return *value;
}

numeric::BigInt RomanSubstitutionGenerator::GetCapacity() const {
    return numeric::BigInt(roman_dictionary_->GetCapacity());
}

//-------------------------------------------------------------
// SpecialSubstitutionGenerator
//-------------------------------------------------------------
SpecialSubstitutionGenerator::SpecialSubstitutionGenerator(const SpecialCharGen& special_gen)
    : min_length_{static_cast<std::uint32_t>(special_gen.min_length)}
    , max_length_{static_cast<std::uint32_t>(special_gen.max_length)} {
    if (min_length_ > constants::kMaxSpecialLength) {
        throw std::runtime_error(fmt::format("Min special symbols length is {}", constants::kMaxSpecialLength));
    }
    if (max_length_ > constants::kMaxSpecialLength) {
        throw std::runtime_error(fmt::format("Max special symbols length is {}", constants::kMaxSpecialLength));
    }
    if (min_length_ > max_length_) {
        throw std::runtime_error("Min special symbols length is greater than max special symbols length");
    }
    cumulative_caps_.resize(max_length_ - min_length_ + 1);
    for (std::size_t i = 0; i < cumulative_caps_.size(); ++i) {
        cumulative_caps_[i] = 1ULL << ((i + min_length_) * 5);
    }
}

std::size_t SpecialSubstitutionGenerator::SelectLength(std::uint32_t seed, std::size_t sequence_number) const {
    auto p = Permute(cumulative_caps_.back(), seed, sequence_number);
    auto it = std::upper_bound(cumulative_caps_.begin(), cumulative_caps_.end(), p);
    auto idx = std::distance(cumulative_caps_.begin(), it);
    return min_length_ + idx;
}

std::string SpecialSubstitutionGenerator::Generate(std::uint32_t seed, std::size_t sequence_number) const {
    std::size_t length = min_length_;
    if (min_length_ != max_length_) {
        // Select a length between min_length_ and max_length_
        // the probability of getting a lenght must be proportional to the length
        length = SelectLength(seed, sequence_number);
    }

    if (length == 0) {
        return "";
    }

    auto index = Permute(1ULL << (length * 5), seed, sequence_number);
    // Now we use each 5 bits to get a special symbol
    std::string result;
    result.resize(length);
    for (std::size_t i = 0; i < length; ++i) {
        result[i] = kSpecialSymbols[index % kSpecialSymbolsCount];
        index /= kSpecialSymbolsCount;
    }
    return result;
}

numeric::BigInt SpecialSubstitutionGenerator::GetCapacity() const {
    return std::accumulate(cumulative_caps_.begin(), cumulative_caps_.end(), numeric::BigInt{0});
}

//-------------------------------------------------------------
// EmojiSubstitutionGeneratorBase
//-------------------------------------------------------------

EmojiSubstitutionGeneratorBase::EmojiSubstitutionGeneratorBase(std::size_t dict_size, const EmojiGen& emoji_gen)
    : dict_size_{dict_size}
    , min_count_{static_cast<std::size_t>(emoji_gen.min_count)}
    , max_count_{static_cast<std::size_t>(emoji_gen.max_count)}
    , unique_{emoji_gen.unique}
    , tone_{emoji_gen.tone}
    , gender_{emoji_gen.gender}
    , cumulative_caps_{} {
    cumulative_caps_.resize(max_count_ - min_count_ + 1);
    if (max_count_ > constants::kMaxEmojiCount) {
        throw DictionaryError("Max count for emoji generator cannot be greater than 16");
    }
    if (unique_) {
        if (dict_size_ < min_count_) {
            throw DictionaryError("Not enough emoji to generate a unique string");
        }
        if (dict_size_ < max_count_) {
            // Adjust max_count_ to the size of the dictionary
            max_count_ = dict_size_;
        }
        for (std::size_t i = 0; i < cumulative_caps_.size(); ++i) {
            cumulative_caps_[i] = UniquePermutationCount(dict_size_, i + min_count_);
        }
    } else {
        for (std::size_t i = 0; i < cumulative_caps_.size(); ++i) {
            cumulative_caps_[i] = PermutationCount(dict_size_, i + min_count_);
        }
    }
}

std::size_t EmojiSubstitutionGeneratorBase::SelectCount(std::uint32_t seed, std::size_t sequence_number) const {
    if (min_count_ == max_count_) {
        return min_count_;
    }
    auto p = Permute(cumulative_caps_.back(), seed, sequence_number);
    auto it = std::upper_bound(cumulative_caps_.begin(), cumulative_caps_.end(), p);
    auto idx = std::distance(cumulative_caps_.begin(), it);
    return min_count_ + idx;
}

std::string EmojiSubstitutionGeneratorBase::Generate(std::uint32_t seed, std::size_t sequence_number) const {
    auto count = SelectCount(seed, sequence_number);
    auto permutation = unique_ ? UniquePermutation(seed, dict_size_, count, sequence_number)
                               : NonUniquePermutation(seed, dict_size_, count, sequence_number);
    std::string result;
    result.reserve(constants::kEmojiMaxCharLength * count);
    for (const auto& item : permutation) {
        result += EmojiAt(item);
    }
    return result;
}

numeric::BigInt EmojiSubstitutionGeneratorBase::GetCapacity() const {
    return std::accumulate(cumulative_caps_.begin(), cumulative_caps_.end(), numeric::BigInt{0});
}

//-------------------------------------------------------------
// AlternationSubstitutionGenerator
//-------------------------------------------------------------
AlternationSubstitutionGenerator::AlternationSubstitutionGenerator(std::vector<SubstitutionGeneratorPtr> children)
    : children_{std::move(children)}
    , max_length_{0} {
    if (children_.empty()) {
        throw std::runtime_error("Alternation must have at least one alternative");
    }
    cumulative_caps_.reserve(children_.size());
    numeric::BigInt total{0};
    for (const auto& child : children_) {
        total += child->GetCapacity();
        // uint64 prefix sums for the block-selection Permute (matching special/emoji cumulative caps).
        cumulative_caps_.push_back(static_cast<std::uint64_t>(total));
        max_length_ = std::max(max_length_, child->GetMaxLength());
    }
    capacity_ = total;
}

std::string AlternationSubstitutionGenerator::Generate(std::uint32_t seed, std::size_t sequence_number) const {
    auto total = cumulative_caps_.back();
    // Permute over the whole sum, then split into (child block, offset within block). The offset
    // becomes the child's sequence, so as the sum's values sweep a block the child sees each of its
    // own indices exactly once -> collision-free.
    auto p = Permute(total, seed, sequence_number);
    auto it = std::upper_bound(cumulative_caps_.begin(), cumulative_caps_.end(), p);
    auto idx = static_cast<std::size_t>(std::distance(cumulative_caps_.begin(), it));
    std::uint64_t block_start = idx == 0 ? 0 : cumulative_caps_[idx - 1];
    std::uint64_t offset = p - block_start;
    return children_[idx]->Generate(seed, offset);
}

//-------------------------------------------------------------
// GroupSubstitutionGenerator
//-------------------------------------------------------------
GroupSubstitutionGenerator::GroupSubstitutionGenerator(
    std::vector<SubstitutionGeneratorPtr> generators,
    Pattern::TextChunks text_chunks
)
    : generators_{std::move(generators)}
    , text_chunks_{std::move(text_chunks)}
    , capacity_{1}
    , max_length_{0} {
    for (const auto& chunk : text_chunks_) {
        max_length_ += chunk.size();
    }
    for (const auto& generator : generators_) {
        // Placeholders inside a group compose like the top-level pattern: LCM capacity.
        capacity_ = lcm(capacity_, generator->GetCapacity());
        max_length_ += generator->GetMaxLength();
    }
}

std::string GroupSubstitutionGenerator::Generate(std::uint32_t seed, std::size_t sequence_number) const {
    // Same composition as PatternGenerator::Impl::Generate: seed-step each placeholder, format with
    // the group's own text chunks (un-escaping literal escapes at generation time).
    std::vector<std::string> substitutions;
    substitutions.reserve(generators_.size());
    for (const auto& generator : generators_) {
        seed += kSeedStep;
        substitutions.push_back(generator->Generate(seed, sequence_number));
    }
    return FormatChunks(text_chunks_, substitutions, /*unescape_text=*/true);
}

//-------------------------------------------------------------
// PatternGenerator::Impl
//-------------------------------------------------------------
namespace {

// Build the selector substitution generator appropriate to the filtered-dictionary type, so
// the otherwise-identical settings/init logic can be shared between the in-memory and binary
// dictionary sets.
auto MakeSelectorGenerator(FilteredDictionaryConstPtr dictionary, const SelectorSettings& settings)
    -> SubstitutionGeneratorPtr {
    return std::make_unique<SelectorSubstitutionGenerator>(std::move(dictionary), settings);
}

auto MakeSelectorGenerator(binary::FilteredDictionaryConstPtr dictionary, const SelectorSettings& settings)
    -> SubstitutionGeneratorPtr {
    return std::make_unique<BinarySelectorSubstitutionGenerator>(std::move(dictionary), settings);
}

// Build the emoji substitution generator appropriate to the filtered-dictionary type. Emoji is a
// regular dictionary kind now, sourced from the dictionary set like any selector.
auto MakeEmojiGenerator(FilteredDictionaryConstPtr dictionary, const EmojiGen& emoji_gen) -> SubstitutionGeneratorPtr {
    return std::make_unique<EmojiSubstitutionGenerator>(std::move(dictionary), emoji_gen);
}

auto MakeEmojiGenerator(binary::FilteredDictionaryConstPtr dictionary, const EmojiGen& emoji_gen)
    -> SubstitutionGeneratorPtr {
    return std::make_unique<BinaryEmojiSubstitutionGenerator>(std::move(dictionary), emoji_gen);
}

// The emoji placeholder filters the "emoji" dictionary kind by its include/exclude tags, exactly
// like a selector; build the equivalent Selector so the shared DictionarySet::Filter path applies.
auto EmojiSelector(const EmojiGen& emoji_gen) -> Selector {
    Selector selector;
    selector.kind = constants::kEmojiKind;
    selector.include_tags = emoji_gen.include_tags;
    selector.exclude_tags = emoji_gen.exclude_tags;
    selector.no_other_tags = emoji_gen.no_other_tags;
    return selector;
}

// Adapts a dictionary set together with the request's enabled opt-in tags into something the build
// helpers can call `.Filter(selector)` on. This lets the "honest opt-in" usage flag ride along
// every filter call without threading it through each helper's signature: the build code keeps
// writing `dictionaries.Filter(selector)`, and the opt-in set is applied underneath.
template <typename DictSet>
struct OptInFilter {
    const DictSet& dictionaries;
    const TagSet& enabled_opt_ins;

    auto Filter(const Selector& selector) const {
        return dictionaries.Filter(selector, enabled_opt_ins);
    }
};

template <typename DictSet>
auto WithOptIns(const DictSet& dictionaries, const TagSet& enabled_opt_ins) -> OptInFilter<DictSet> {
    return OptInFilter<DictSet>{dictionaries, enabled_opt_ins};
}

// Build a generator for one simple (non-alternating) placeholder. Used for alternation children.
// Unlike the top-level path, selectors here use the plain dictionary size (no prime/LCM
// maximization, which is a flat-composition heuristic; children compose by sum) and are not stored
// in PatternSettings::selectors, so the settings format for alternation-free patterns is unchanged.
template <typename DictSet>
auto BuildSimpleGenerator(const DictSet& dictionaries, const Pattern::SimplePlaceholder& element)
    -> SubstitutionGeneratorPtr {
    if (std::holds_alternative<Selector>(element)) {
        const auto& selector = std::get<Selector>(element);
        auto filtered_dict = dictionaries.Filter(selector);
        if (!filtered_dict || filtered_dict->empty()) {
            throw PatternSyntaxError("No matching words found for: " + selector.ToString());
        }
        auto original_size = static_cast<std::int64_t>(filtered_dict->size());
        auto selected_size = filtered_dict->GetCase() == CaseType::kMixed
                                 ? static_cast<std::int64_t>(filtered_dict->MixedCapacity())
                                 : original_size;
        return MakeSelectorGenerator(filtered_dict, SelectorSettings{original_size, selected_size});
    }
    if (std::holds_alternative<NumberGen>(element)) {
        const auto& number_gen = std::get<NumberGen>(element);
        if (number_gen.base == NumberBase::kRoman || number_gen.base == NumberBase::kRomanLower) {
            return std::make_unique<RomanSubstitutionGenerator>(number_gen);
        }
        return std::make_unique<NumberSubstitutionGenerator>(number_gen);
    }
    if (std::holds_alternative<SpecialCharGen>(element)) {
        return std::make_unique<SpecialSubstitutionGenerator>(std::get<SpecialCharGen>(element));
    }
    const auto& emoji_gen = std::get<EmojiGen>(element);
    auto emoji_dict = dictionaries.Filter(EmojiSelector(emoji_gen));
    if (!emoji_dict || emoji_dict->empty()) {
        throw PatternSyntaxError("No matching emoji found for: " + emoji_gen.ToString());
    }
    return MakeEmojiGenerator(emoji_dict, emoji_gen);
}

// Enumerate a generator's full output set, or nullopt if it is too large to enumerate.
auto EnumerateOutputs(const SubstitutionGenerator& generator) -> std::optional<std::unordered_set<std::string>> {
    constexpr std::uint64_t kMaxEnumerate = 100000;
    if (generator.GetCapacity() > numeric::BigInt(kMaxEnumerate)) {
        return std::nullopt;
    }
    auto count = static_cast<std::uint64_t>(generator.GetCapacity());
    std::unordered_set<std::string> outputs;
    outputs.reserve(count);
    for (std::uint64_t s = 0; s < count; ++s) {
        outputs.insert(generator.Generate(0, s));
    }
    return outputs;
}

bool SetsOverlap(const std::unordered_set<std::string>& a, const std::unordered_set<std::string>& b) {
    const auto& small = a.size() <= b.size() ? a : b;
    const auto& large = a.size() <= b.size() ? b : a;
    for (const auto& value : small) {
        if (large.count(value) != 0) {
            return true;
        }
    }
    return false;
}

enum class Disjointness { kDisjoint, kOverlap, kUnknown };

// Do two placeholders' output sets overlap? kUnknown if either is too large to enumerate.
template <typename DictSet>
auto PlaceholderDisjointness(
    const DictSet& dictionaries,
    const Pattern::SimplePlaceholder& a,
    const Pattern::SimplePlaceholder& b
) -> Disjointness {
    auto set_a = EnumerateOutputs(*BuildSimpleGenerator(dictionaries, a));
    auto set_b = EnumerateOutputs(*BuildSimpleGenerator(dictionaries, b));
    if (!set_a || !set_b) {
        return Disjointness::kUnknown;
    }
    return SetsOverlap(*set_a, *set_b) ? Disjointness::kOverlap : Disjointness::kDisjoint;
}

std::string UnescapeChunk(const std::string& chunk) {
    std::string result;
    for (std::size_t i = 0; i < chunk.size(); ++i) {
        if (chunk[i] == '\\' && i + 1 < chunk.size()) {
            result.push_back(chunk[++i]);
        } else {
            result.push_back(chunk[i]);
        }
    }
    return result;
}

// Can two branch groups produce the same slug? Same-shape branches use the per-position refinement
// (a product of positions is empty iff any factor is empty), which only enumerates each placeholder
// (cheap) rather than the whole cross-product. Different-shape branches fall back to bounded
// full-output enumeration. Pairs that are too large to decide are treated as non-overlapping.
template <typename DictSet>
bool GroupsOverlap(
    const DictSet& dictionaries,
    const Pattern::Group& g1,
    const SubstitutionGenerator& gen1,
    const Pattern::Group& g2,
    const SubstitutionGenerator& gen2
) {
    if (g1.placeholders.size() == g2.placeholders.size()) {
        for (std::size_t k = 0; k < g1.text_chunks.size(); ++k) {
            if (UnescapeChunk(g1.text_chunks[k]) != UnescapeChunk(g2.text_chunks[k])) {
                return false;  // differing literal text separates the branches
            }
        }
        bool any_unknown = false;
        for (std::size_t i = 0; i < g1.placeholders.size(); ++i) {
            switch (PlaceholderDisjointness(dictionaries, g1.placeholders[i], g2.placeholders[i])) {
                case Disjointness::kDisjoint:
                    return false;  // this position separates the branches
                case Disjointness::kUnknown:
                    any_unknown = true;
                    break;
                case Disjointness::kOverlap:
                    break;
            }
        }
        return !any_unknown;  // all positions overlap (text equal) -> overlap; unknown -> best-effort allow
    }
    // Different shape: bounded full-output enumeration.
    auto set1 = EnumerateOutputs(gen1);
    auto set2 = EnumerateOutputs(gen2);
    if (!set1 || !set2) {
        return false;
    }
    return SetsOverlap(*set1, *set2);
}

// Validation sweep: alternation branches must be pairwise disjoint, else the alternation is not
// collision-free (two branches producing the same slug -- a word that is both a noun and a verb, or
// a superset like `{adverb}` overlapping `{adverb:+pos}`).
template <typename DictSet>
void CheckAlternativesDisjoint(
    const DictSet& dictionaries,
    const std::vector<SubstitutionGeneratorPtr>& children,
    const Pattern::Alternation& alternation
) {
    for (std::size_t i = 0; i < alternation.alternatives.size(); ++i) {
        for (std::size_t j = i + 1; j < alternation.alternatives.size(); ++j) {
            if (GroupsOverlap(
                    dictionaries, alternation.alternatives[i], *children[i], alternation.alternatives[j], *children[j]
                )) {
                throw PatternSyntaxError(fmt::format(
                    "Alternation branches `{}` and `{}` overlap: they can produce the same slug",
                    alternation.alternatives[i].ToString(),
                    alternation.alternatives[j].ToString()
                ));
            }
        }
    }
}

template <typename DictSet>
auto BuildGroupGenerator(const DictSet& dictionaries, const Pattern::Group& group) -> SubstitutionGeneratorPtr {
    // A bare single placeholder with no surrounding text is exactly that placeholder -- build it
    // directly (no group wrapper), so placeholder alternation `{a}|{b}` keeps its seed stepping and
    // stays byte-identical to before groups existed.
    if (group.placeholders.size() == 1 && group.text_chunks[0].empty() && group.text_chunks[1].empty()) {
        return BuildSimpleGenerator(dictionaries, group.placeholders[0]);
    }
    std::vector<SubstitutionGeneratorPtr> generators;
    generators.reserve(group.placeholders.size());
    for (const auto& placeholder : group.placeholders) {
        generators.push_back(BuildSimpleGenerator(dictionaries, placeholder));
    }
    return std::make_unique<GroupSubstitutionGenerator>(std::move(generators), group.text_chunks);
}

template <typename DictSet>
auto BuildAlternationGenerator(const DictSet& dictionaries, const Pattern::Alternation& alternation, bool validate)
    -> SubstitutionGeneratorPtr {
    std::vector<SubstitutionGeneratorPtr> children;
    children.reserve(alternation.alternatives.size());
    for (const auto& branch : alternation.alternatives) {
        children.push_back(BuildGroupGenerator(dictionaries, branch));
    }
    if (validate) {
        CheckAlternativesDisjoint(dictionaries, children, alternation);
    }
    return std::make_unique<AlternationSubstitutionGenerator>(std::move(children));
}

}  // namespace

struct PatternGenerator::Impl {
    PatternPtr pattern;
    std::string seed;
    std::vector<SubstitutionGeneratorPtr> generators;
    PatternSettings settings;

    // Constructor for the case when settings are not calculated yet for the pattern
    Impl(const DictionarySet& dictionaries, PatternPtr pattern, const TagSet& enabled_opt_ins)
        : pattern{pattern}
        , generators{}
        , settings{CalculateSettings(WithOptIns(dictionaries, enabled_opt_ins))} {
        //
    }

    // Constructor for the case when settings are provided
    Impl(const DictionarySet& dictionaries, PatternPtr pattern, PatternSettings settings, const TagSet& enabled_opt_ins)
        : pattern{pattern}
        , generators{}
        , settings{settings} {
        InitGenerators(WithOptIns(dictionaries, enabled_opt_ins));
    }

    // Binary dictionary set overloads: the binary dictionaries reproduce the in-memory
    // dictionaries' lexicographic order, so the same settings/init logic applies and the
    // generated slugs are byte-identical.
    Impl(const binary::DictionarySet& dictionaries, PatternPtr pattern, const TagSet& enabled_opt_ins)
        : pattern{pattern}
        , generators{}
        , settings{CalculateSettings(WithOptIns(dictionaries, enabled_opt_ins))} {
        //
    }

    Impl(
        const binary::DictionarySet& dictionaries,
        PatternPtr pattern,
        PatternSettings settings,
        const TagSet& enabled_opt_ins
    )
        : pattern{pattern}
        , generators{}
        , settings{settings} {
        InitGenerators(WithOptIns(dictionaries, enabled_opt_ins));
    }

    // This function has a side effect of initializing the generators
    template <typename DictSet>
    PatternSettings CalculateSettings(const DictSet& dictionaries) {
        numeric::BigInt capacity(1);
        std::size_t max_pattern_length = pattern->ArbitraryTextLength();

        std::vector<SelectorSettings> selectors;
        using FilteredPtr = decltype(dictionaries.Filter(std::declval<const Selector&>()));
        std::map<std::int64_t, FilteredPtr> filtered_dictionaries;

        for (const auto& element : pattern->placeholders) {
            if (std::holds_alternative<Selector>(element)) {
                const auto& selector = std::get<Selector>(element);
                auto hash = selector.GetHash();
                auto f = filtered_dictionaries.find(hash);
                if (f == filtered_dictionaries.end()) {
                    f = filtered_dictionaries.emplace(hash, dictionaries.Filter(selector)).first;
                }
                auto filtered_dict = f->second;
                if (!filtered_dict || filtered_dict->empty()) {
                    throw PatternSyntaxError("No matching words found for: " + selector.ToString());
                }
                auto original_size = filtered_dict->size();
                SelectorSettings settings{
                    static_cast<std::int64_t>(original_size), static_cast<std::int64_t>(original_size)
                };
                if (filtered_dict->GetCase() == CaseType::kMixed) {
                    // Mixed case multiplies a word's capacity by its number of cased forms; use the
                    // collision-free block layout's total instead of the plain word count.
                    settings.selected_size = static_cast<std::int64_t>(filtered_dict->MixedCapacity());
                } else {
                    // use primes to maximize capacity
                    auto original_capacity = lcm(capacity, numeric::BigInt(original_size));
                    if (original_size > 2) {
                        auto prime = utils::PrevPrime(original_size);
                        auto prime_capacity = lcm(capacity, numeric::BigInt(prime));
                        if (prime_capacity > original_capacity) {
                            settings.selected_size = prime;
                        }
                    }
                }
                selectors.push_back(settings);
                generators.push_back(MakeSelectorGenerator(filtered_dict, settings));
            } else if (std::holds_alternative<NumberGen>(element)) {
                const auto& number_gen = std::get<NumberGen>(element);
                if (number_gen.base == NumberBase::kRoman || number_gen.base == NumberBase::kRomanLower) {
                    generators.push_back(std::make_unique<RomanSubstitutionGenerator>(number_gen));
                } else {
                    generators.push_back(std::make_unique<NumberSubstitutionGenerator>(number_gen));
                }
            } else if (std::holds_alternative<SpecialCharGen>(element)) {
                const auto& special_gen = std::get<SpecialCharGen>(element);
                generators.push_back(std::make_unique<SpecialSubstitutionGenerator>(special_gen));
            } else if (std::holds_alternative<EmojiGen>(element)) {
                const auto& emoji_gen = std::get<EmojiGen>(element);
                auto emoji_dict = dictionaries.Filter(EmojiSelector(emoji_gen));
                if (!emoji_dict || emoji_dict->empty()) {
                    throw PatternSyntaxError("No matching emoji found for: " + emoji_gen.ToString());
                }
                generators.push_back(MakeEmojiGenerator(emoji_dict, emoji_gen));
            } else if (std::holds_alternative<Pattern::Alternation>(element)) {
                // Validation sweep: check the alternatives' outputs are disjoint (last, like the
                // per-selector "no matching words" check).
                generators.push_back(BuildAlternationGenerator(
                    dictionaries, std::get<Pattern::Alternation>(element), /*validate=*/true
                ));
            }
            capacity = lcm(capacity, generators.back()->GetCapacity());
            max_pattern_length += generators.back()->GetMaxLength();
        }
        return PatternSettings{selectors, capacity, static_cast<std::uint8_t>(max_pattern_length)};
    }

    template <typename DictSet>
    void InitGenerators(const DictSet& dictionaries) {
        auto selector_settings = settings.selectors.begin();
        numeric::BigInt capacity{1};
        std::size_t max_pattern_length = pattern->ArbitraryTextLength();
        for (const auto& element : pattern->placeholders) {
            if (std::holds_alternative<Selector>(element)) {
                if (selector_settings == settings.selectors.end()) {
                    throw std::runtime_error("Incorrect pattern settings");
                }
                const auto& selector = std::get<Selector>(element);
                auto filtered_dict = dictionaries.Filter(selector);
                if (!filtered_dict || filtered_dict->empty()) {
                    throw PatternSyntaxError("No matching words found for: " + selector.ToString());
                }
                generators.push_back(MakeSelectorGenerator(filtered_dict, *selector_settings));
                ++selector_settings;
            } else if (std::holds_alternative<NumberGen>(element)) {
                const auto& number_gen = std::get<NumberGen>(element);
                if (number_gen.base == NumberBase::kRoman || number_gen.base == NumberBase::kRomanLower) {
                    generators.push_back(std::make_unique<RomanSubstitutionGenerator>(number_gen));
                } else {
                    generators.push_back(std::make_unique<NumberSubstitutionGenerator>(number_gen));
                }
            } else if (std::holds_alternative<SpecialCharGen>(element)) {
                const auto& special_gen = std::get<SpecialCharGen>(element);
                generators.push_back(std::make_unique<SpecialSubstitutionGenerator>(special_gen));
            } else if (std::holds_alternative<EmojiGen>(element)) {
                const auto& emoji_gen = std::get<EmojiGen>(element);
                auto emoji_dict = dictionaries.Filter(EmojiSelector(emoji_gen));
                if (!emoji_dict || emoji_dict->empty()) {
                    throw PatternSyntaxError("No matching emoji found for: " + emoji_gen.ToString());
                }
                generators.push_back(MakeEmojiGenerator(emoji_dict, emoji_gen));
            } else if (std::holds_alternative<Pattern::Alternation>(element)) {
                // Loading stored settings: the pattern was already validated when the settings were
                // computed, so skip the (costly) disjointness enumeration here.
                generators.push_back(BuildAlternationGenerator(
                    dictionaries, std::get<Pattern::Alternation>(element), /*validate=*/false
                ));
            }
            capacity = lcm(capacity, generators.back()->GetCapacity());
            max_pattern_length += generators.back()->GetMaxLength();
        }
        settings.capacity = capacity;
        settings.max_pattern_length = max_pattern_length;
    }

    std::string Generate(std::string_view seed, std::size_t sequence_number) const {
        return Generate(FNV1aHash(seed), sequence_number);
    }

    std::string Generate(std::uint32_t seed, std::size_t sequence_number) const {
        std::vector<std::string> substitutions;
        for (const auto& generator : generators) {
            seed += kSeedStep;
            substitutions.push_back(generator->Generate(seed, sequence_number));
        }
        return pattern->Format(substitutions);
    }
};

PatternGenerator::PatternGenerator(
    const DictionarySet& dictionaries,
    PatternPtr pattern,
    const TagSet& enabled_opt_ins
)
    : impl_{dictionaries, pattern, enabled_opt_ins} {
}

PatternGenerator::PatternGenerator(
    const DictionarySet& dictionaries,
    PatternPtr pattern,
    PatternSettings settings,
    const TagSet& enabled_opt_ins
)
    : impl_{dictionaries, pattern, settings, enabled_opt_ins} {
}

PatternGenerator::PatternGenerator(
    const binary::DictionarySet& dictionaries,
    PatternPtr pattern,
    const TagSet& enabled_opt_ins
)
    : impl_{dictionaries, pattern, enabled_opt_ins} {
}

PatternGenerator::PatternGenerator(
    const binary::DictionarySet& dictionaries,
    PatternPtr pattern,
    PatternSettings settings,
    const TagSet& enabled_opt_ins
)
    : impl_{dictionaries, pattern, settings, enabled_opt_ins} {
}

PatternGenerator::~PatternGenerator() = default;

std::string PatternGenerator::operator()(std::uint32_t seed, std::size_t sequence_number) const {
    return impl_->Generate(seed, sequence_number);
}

std::string PatternGenerator::operator()(std::string_view seed, std::size_t sequence_number) const {
    return impl_->Generate(seed, sequence_number);
}

numeric::BigInt PatternGenerator::GetCapacity() const {
    return impl_->settings.capacity;
}

std::size_t PatternGenerator::GetMaxPatternLength() const {
    return impl_->settings.max_pattern_length;
}

const PatternSettings& PatternGenerator::GetSettings() const {
    return impl_->settings;
}

std::uint32_t PatternGenerator::SeedHash(std::string_view seed) {
    return FNV1aHash(seed);
}

}  // namespace slugkit::generator
