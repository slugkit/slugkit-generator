#include <benchmark/benchmark.h>

#include <slugkit/generator/pattern.hpp>

namespace slugkit::generator::benchmarks {

void ParseNumberPlaceholder(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto pattern = Pattern{"{number:8d}"};
        benchmark::DoNotOptimize(pattern.pattern);
    }
}

void ParseSpecialFixedPlaceholder(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto pattern = Pattern{"{special:8}"};
        benchmark::DoNotOptimize(pattern.pattern);
    }
}

void ParseSpecialRangePlaceholder(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto pattern = Pattern{"{special:8-12}"};
        benchmark::DoNotOptimize(pattern.pattern);
    }
}

void ParseSelector(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto pattern = Pattern{"{adjective}"};
        benchmark::DoNotOptimize(pattern.pattern);
    }
}

void ParseSelectorUppercase(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto pattern = Pattern{"{ADJECTIVE}"};
        benchmark::DoNotOptimize(pattern.pattern);
    }
}

void ParseSelectorTitlecase(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto pattern = Pattern{"{Adjective}"};
        benchmark::DoNotOptimize(pattern.pattern);
    }
}

void ParseSelectorMixedcase(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto pattern = Pattern{"{aDjective}"};
        benchmark::DoNotOptimize(pattern.pattern);
    }
}

void ParseSelectorWithTag(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto pattern = Pattern{"{adjective:+tag}"};
        benchmark::DoNotOptimize(pattern.pattern);
    }
}

void ParseSelectorWithTags(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto pattern = Pattern{"{adjective:+tag1-tag2}"};
        benchmark::DoNotOptimize(pattern.pattern);
    }
}

void ParseSelectorWithSize(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto pattern = Pattern{"{adjective:==10}"};
        benchmark::DoNotOptimize(pattern.pattern);
    }
}

void ParseSelectorWithSizeAndTags(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto pattern = Pattern{"{adjective:+tag1-tag2==10}"};
        benchmark::DoNotOptimize(pattern.pattern);
    }
}

void ParsePattern2Components(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto pattern = Pattern{"{adjective}-{noun}"};
        benchmark::DoNotOptimize(pattern.pattern);
    }
}

void ParsePattern3Components(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto pattern = Pattern{"{adjective}-{noun}-{verb}"};
        benchmark::DoNotOptimize(pattern.pattern);
    }
}

void ParsePattern4Components(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto pattern = Pattern{"{adverb}-{adjective}-{noun}-{verb}"};
        benchmark::DoNotOptimize(pattern.pattern);
    }
}

void ParseDemoPattern(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto pattern = Pattern{"{adverb}-{adjective}-{noun}-{number:4x}"};
        benchmark::DoNotOptimize(pattern.pattern);
    }
}

BENCHMARK(ParseNumberPlaceholder);
BENCHMARK(ParseSpecialFixedPlaceholder);
BENCHMARK(ParseSpecialRangePlaceholder);
BENCHMARK(ParseSelector);
BENCHMARK(ParseSelectorUppercase);
BENCHMARK(ParseSelectorTitlecase);
BENCHMARK(ParseSelectorMixedcase);
BENCHMARK(ParseSelectorWithTag);
BENCHMARK(ParseSelectorWithTags);
BENCHMARK(ParseSelectorWithSize);
BENCHMARK(ParseSelectorWithSizeAndTags);
BENCHMARK(ParsePattern2Components);
BENCHMARK(ParsePattern3Components);
BENCHMARK(ParsePattern4Components);
BENCHMARK(ParseDemoPattern);

}  // namespace slugkit::generator::benchmarks