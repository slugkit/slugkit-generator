#include <slugkit/generator/generator.hpp>

#include <slugkit/generator/binary_dictionary.hpp>
#include <slugkit/generator/exceptions.hpp>
#include <slugkit/generator/pattern_generator.hpp>

#include <random>
#include <variant>

namespace slugkit::generator {

struct Generator::Impl {
    // The generator works with either an in-memory or a binary (memory-mapped) dictionary
    // set; PatternGenerator accepts both and produces byte-identical slugs.
    std::variant<DictionarySet, binary::DictionarySet> dictionaries;

    PatternSettings GetCapacity(PatternPtr pattern) const {
        // TODO LRU cache for pattern generators
        return std::visit(
            [&](const auto& dict) { return PatternGenerator(dict, pattern).GetSettings(); }, dictionaries
        );
    }

    std::string Generate(std::string_view pattern_str, std::string_view seed, std::size_t sequence_number) const {
        auto pattern = std::make_shared<Pattern>(std::string(pattern_str));
        return Generate(pattern, seed, sequence_number);
    }

    std::string Generate(PatternPtr pattern, std::string_view seed, std::size_t sequence_number) const {
        return std::visit(
            [&](const auto& dict) {
                auto generator = PatternGenerator(dict, pattern);
                return generator(seed, sequence_number);
            },
            dictionaries
        );
    }

    std::string Generate(
        const PatternSettings& settings,
        PatternPtr pattern,
        std::string_view seed,
        std::size_t sequence_number
    ) const {
        // TODO LRU cache for pattern generators
        return std::visit(
            [&](const auto& dict) {
                auto generator = PatternGenerator(dict, pattern, settings);
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
        GenerateCallback callback
    ) const {
        auto pattern = std::make_shared<Pattern>(std::string(pattern_str));
        Generate(pattern, seed, sequence_number, count, callback);
    }

    void Generate(
        PatternPtr pattern,
        std::string_view seed,
        std::size_t sequence_number,
        std::size_t count,
        GenerateCallback callback
    ) const {
        std::visit(
            [&](const auto& dict) {
                auto generator = PatternGenerator(dict, pattern);
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
        GenerateCallback callback
    ) const {
        // TODO LRU cache for pattern generators
        std::visit(
            [&](const auto& dict) {
                auto generator = PatternGenerator(dict, pattern, settings);
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

PatternSettings Generator::GetCapacity(std::string_view pattern_str) const {
    auto pattern = std::make_shared<Pattern>(std::string(pattern_str));
    return GetCapacity(pattern);
}

PatternSettings Generator::GetCapacity(PatternPtr pattern) const {
    return impl_->GetCapacity(pattern);
}

std::string Generator::Generate(std::string_view pattern_str, std::string_view seed, std::size_t sequence_number)
    const {
    return impl_->Generate(pattern_str, seed, sequence_number);
}

std::string Generator::Generate(PatternPtr pattern, std::string_view seed, std::size_t sequence_number) const {
    return impl_->Generate(pattern, seed, sequence_number);
}

std::string Generator::Generate(
    const PatternSettings& settings,
    PatternPtr pattern,
    std::string_view seed,
    std::size_t sequence_number
) const {
    return impl_->Generate(settings, pattern, seed, sequence_number);
}

void Generator::Generate(
    const PatternSettings& settings,
    PatternPtr pattern,
    std::string_view seed,
    std::size_t sequence_number,
    std::size_t count,
    GenerateCallback callback
) const {
    return impl_->Generate(settings, pattern, seed, sequence_number, count, callback);
}

void Generator::Generate(
    PatternPtr pattern,
    std::string_view seed,
    std::size_t sequence_number,
    std::size_t count,
    GenerateCallback callback
) const {
    return impl_->Generate(pattern, seed, sequence_number, count, callback);
}

void Generator::Generate(
    std::string_view pattern_str,
    std::string_view seed,
    std::size_t sequence_number,
    std::size_t count,
    GenerateCallback callback
) const {
    return impl_->Generate(pattern_str, seed, sequence_number, count, callback);
}

}  // namespace slugkit::generator
