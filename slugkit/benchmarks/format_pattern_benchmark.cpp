#include <benchmark/benchmark.h>

#include <slugkit/generator/pattern.hpp>

namespace slugkit::generator::benchmarks {

using namespace literals;

namespace {

constexpr std::string_view kPlaceholder = "{adjective}";
auto constexpr kSubstitution = "chlorobenzylidenemalononitrile";

auto GeneratePattern(std::int64_t num_components) -> PatternPtr {
    std::string pattern_str{kPlaceholder};
    auto size = num_components * kPlaceholder.size() + num_components - 1;
    pattern_str.reserve(size);
    for (std::int64_t i = 1; i < num_components; ++i) {
        pattern_str.append("-").append(kPlaceholder);
    }
    return std::make_shared<Pattern>(pattern_str);
}

auto GenerateSubstitutions(std::int64_t num_components) -> Pattern::Substitutions {
    Pattern::Substitutions substitutions;
    for (std::int64_t i = 0; i < num_components; ++i) {
        substitutions.push_back(kSubstitution);
    }
    return substitutions;
}

}  // namespace

void FormatPattern(benchmark::State& state) {
    auto pattern = GeneratePattern(state.range(0));
    auto substitutions = GenerateSubstitutions(state.range(0));
    state.SetLabel(fmt::format("{} components", state.range(0)));
    for ([[maybe_unused]] auto _ : state) {
        auto result = pattern->Format(substitutions);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(FormatPattern)->DenseRange(1, 10);

}  // namespace slugkit::generator::benchmarks
