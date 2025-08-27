#include <benchmark/benchmark.h>

#include <slugkit/generator/pattern.hpp>

namespace slugkit::generator::benchmarks {

using namespace literals;

auto constexpr kSubstitution = "chlorobenzylidenemalononitrile";

void FormatPattern1Component(benchmark::State& state) {
    auto pattern = "{adjective}"_pattern;
    Pattern::Substitutions substitutions{kSubstitution};
    for ([[maybe_unused]] auto _ : state) {
        auto result = pattern.Format(substitutions);
        benchmark::DoNotOptimize(result);
    }
}

void FormatPattern2Components(benchmark::State& state) {
    auto pattern = "{adjective}-{noun}"_pattern;
    Pattern::Substitutions substitutions{kSubstitution, kSubstitution};
    for ([[maybe_unused]] auto _ : state) {
        auto result = pattern.Format(substitutions);
        benchmark::DoNotOptimize(result);
    }
}

void FormatPattern3Components(benchmark::State& state) {
    auto pattern = "{adjective}-{noun}-{verb}"_pattern;
    Pattern::Substitutions substitutions{kSubstitution, kSubstitution, kSubstitution};
    for ([[maybe_unused]] auto _ : state) {
        auto result = pattern.Format(substitutions);
        benchmark::DoNotOptimize(result);
    }
}

void FormatPattern4Components(benchmark::State& state) {
    auto pattern = "{adjective}-{noun}-{verb}-{adjective}"_pattern;
    Pattern::Substitutions substitutions{kSubstitution, kSubstitution, kSubstitution, kSubstitution};
    for ([[maybe_unused]] auto _ : state) {
        auto result = pattern.Format(substitutions);
        benchmark::DoNotOptimize(result);
    }
}

void FormatPattern10Components(benchmark::State& state) {
    auto pattern = "{adjective}-{noun}-{verb}-{adjective}-{noun}-{verb}-{adjective}-{noun}-{verb}-{adjective}"_pattern;
    Pattern::Substitutions substitutions{
        kSubstitution,
        kSubstitution,
        kSubstitution,
        kSubstitution,
        kSubstitution,
        kSubstitution,
        kSubstitution,
        kSubstitution,
        kSubstitution,
        kSubstitution
    };
    for ([[maybe_unused]] auto _ : state) {
        auto result = pattern.Format(substitutions);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(FormatPattern1Component);
BENCHMARK(FormatPattern2Components);
BENCHMARK(FormatPattern3Components);
BENCHMARK(FormatPattern4Components);
BENCHMARK(FormatPattern10Components);

}  // namespace slugkit::generator::benchmarks
