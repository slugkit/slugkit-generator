#include <slugkit/generator/generator.hpp>

#include <slugkit/generator/binary_dictionary.hpp>
#include <slugkit/generator/exceptions.hpp>
#include <slugkit/generator/pattern_generator.hpp>

#include <random>
#include <variant>

namespace slugkit::generator {

namespace {
// Build a TagSet (views) over the generator's owning opt-in strings. std::set nodes are stable,
// so the views stay valid while the source set is unchanged -- long enough for a filter call.
auto MakeOptInView(const std::set<std::string>& enabled_opt_ins) -> TagSet {
    TagSet view;
    for (const auto& tag : enabled_opt_ins) {
        view.emplace(std::string_view{tag});
    }
    return view;
}
}  // namespace

struct Generator::Impl {
    // The generator works with either an in-memory or a binary (memory-mapped) dictionary
    // set; PatternGenerator accepts both and produces byte-identical slugs.
    std::variant<DictionarySet, binary::DictionarySet> dictionaries;

    PatternSettings GetCapacity(PatternPtr pattern, const TagSet& enabled_opt_ins) const {
        // TODO LRU cache for pattern generators
        return std::visit(
            [&](const auto& dict) { return PatternGenerator(dict, pattern, enabled_opt_ins).GetSettings(); },
            dictionaries
        );
    }

    std::string Generate(
        std::string_view pattern_str,
        std::string_view seed,
        std::size_t sequence_number,
        const TagSet& enabled_opt_ins
    ) const {
        auto pattern = std::make_shared<Pattern>(std::string(pattern_str));
        return Generate(pattern, seed, sequence_number, enabled_opt_ins);
    }

    std::string Generate(
        PatternPtr pattern,
        std::string_view seed,
        std::size_t sequence_number,
        const TagSet& enabled_opt_ins
    ) const {
        return std::visit(
            [&](const auto& dict) {
                auto generator = PatternGenerator(dict, pattern, enabled_opt_ins);
                return generator(seed, sequence_number);
            },
            dictionaries
        );
    }

    std::string Generate(
        const PatternSettings& settings,
        PatternPtr pattern,
        std::string_view seed,
        std::size_t sequence_number,
        const TagSet& enabled_opt_ins
    ) const {
        // TODO LRU cache for pattern generators
        return std::visit(
            [&](const auto& dict) {
                auto generator = PatternGenerator(dict, pattern, settings, enabled_opt_ins);
                return generator(seed, sequence_number);
            },
            dictionaries
        );
    }

    void Generate(
        std::string_view pattern_str,
        std::string_view seed,
        std::size_t sequence_number,
        std::size_t count,
        GenerateCallback callback,
        const TagSet& enabled_opt_ins
    ) const {
        auto pattern = std::make_shared<Pattern>(std::string(pattern_str));
        Generate(pattern, seed, sequence_number, count, callback, enabled_opt_ins);
    }

    void Generate(
        PatternPtr pattern,
        std::string_view seed,
        std::size_t sequence_number,
        std::size_t count,
        GenerateCallback callback,
        const TagSet& enabled_opt_ins
    ) const {
        std::visit(
            [&](const auto& dict) {
                auto generator = PatternGenerator(dict, pattern, enabled_opt_ins);
                auto seed_hash = PatternGenerator::SeedHash(seed);
                for (std::size_t i = 0; i < count; ++i) {
                    callback(generator(seed_hash, sequence_number + i));
                }
            },
            dictionaries
        );
    }

    void Generate(
        const PatternSettings& settings,
        PatternPtr pattern,
        std::string_view seed,
        std::size_t sequence_number,
        std::size_t count,
        GenerateCallback callback,
        const TagSet& enabled_opt_ins
    ) const {
        // TODO LRU cache for pattern generators
        std::visit(
            [&](const auto& dict) {
                auto generator = PatternGenerator(dict, pattern, settings, enabled_opt_ins);
                auto seed_hash = PatternGenerator::SeedHash(seed);
                for (std::size_t i = 0; i < count; ++i) {
                    callback(generator(seed_hash, sequence_number + i));
                }
            },
            dictionaries
        );
    }
};

//--------------------------------

Generator::Generator(DictionarySet dictionaries)
    : impl_{std::move(dictionaries)} {
}

Generator::Generator(binary::DictionarySet dictionaries)
    : impl_{std::move(dictionaries)} {
}

Generator::~Generator() noexcept = default;

std::string Generator::RandomSeed() const {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<std::uint32_t> distribution(0, std::numeric_limits<std::uint32_t>::max());
    return fmt::format("{:08x}", distribution(rng));
}

void Generator::EnableOptIn(std::string_view tag) {
    enabled_opt_ins_.emplace(tag);
}

void Generator::SetEnabledOptIns(std::vector<std::string> tags) {
    enabled_opt_ins_ = std::set<std::string>(std::make_move_iterator(tags.begin()), std::make_move_iterator(tags.end()));
}

void Generator::ClearOptIns() {
    enabled_opt_ins_.clear();
}

std::vector<std::string> Generator::EnabledOptIns() const {
    return std::vector<std::string>(enabled_opt_ins_.begin(), enabled_opt_ins_.end());
}

PatternSettings Generator::GetCapacity(std::string_view pattern_str) const {
    auto pattern = std::make_shared<Pattern>(std::string(pattern_str));
    return GetCapacity(pattern);
}

PatternSettings Generator::GetCapacity(PatternPtr pattern) const {
    return impl_->GetCapacity(pattern, MakeOptInView(enabled_opt_ins_));
}

std::string Generator::Generate(std::string_view pattern_str, std::string_view seed, std::size_t sequence_number)
    const {
    return impl_->Generate(pattern_str, seed, sequence_number, MakeOptInView(enabled_opt_ins_));
}

std::string Generator::Generate(PatternPtr pattern, std::string_view seed, std::size_t sequence_number) const {
    return impl_->Generate(pattern, seed, sequence_number, MakeOptInView(enabled_opt_ins_));
}

std::string Generator::Generate(
    const PatternSettings& settings,
    PatternPtr pattern,
    std::string_view seed,
    std::size_t sequence_number
) const {
    return impl_->Generate(settings, pattern, seed, sequence_number, MakeOptInView(enabled_opt_ins_));
}

void Generator::Generate(
    const PatternSettings& settings,
    PatternPtr pattern,
    std::string_view seed,
    std::size_t sequence_number,
    std::size_t count,
    GenerateCallback callback
) const {
    return impl_->Generate(settings, pattern, seed, sequence_number, count, callback, MakeOptInView(enabled_opt_ins_));
}

void Generator::Generate(
    PatternPtr pattern,
    std::string_view seed,
    std::size_t sequence_number,
    std::size_t count,
    GenerateCallback callback
) const {
    return impl_->Generate(pattern, seed, sequence_number, count, callback, MakeOptInView(enabled_opt_ins_));
}

void Generator::Generate(
    std::string_view pattern_str,
    std::string_view seed,
    std::size_t sequence_number,
    std::size_t count,
    GenerateCallback callback
) const {
    return impl_->Generate(pattern_str, seed, sequence_number, count, callback, MakeOptInView(enabled_opt_ins_));
}

}  // namespace slugkit::generator
