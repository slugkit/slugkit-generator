#!/usr/bin/env python3

import argparse
import re
import sys
import os

from typing import Self

TABLE_HEADER = """
| Benchmark                                     | Time  |           CPU | Iterations |     |
| ----                                          | ---   | ---           | ---        | --- |
"""

DETAIL_BLOCK = """

<details>
<summary>Benchmark results</summary>

{content}

</details>

"""


BENCHMARK_GROUPS = {
    re.compile("^--+$"): "line",
    re.compile("FNV1aHash.*"): "Seed Hash Calculation",
    re.compile("Permute.*"): "Permutation",
    re.compile("Parse.*"): "Pattern Parsing",
    re.compile("Format.*"): "Pattern Formatting",
    re.compile("Build.*"): "Dictionary Building",
    re.compile("GenerateSlug.*"): "Slug Generation",
    re.compile("Generate.*"): "Value Generation",
    re.compile("Query.*"): "IndexQuery",
    re.compile("Estimate.*"): "Index Estimation",
    re.compile("FilterDictionary.*"): "Dictionary Filtering",
    re.compile("Calculate.*"): "Pattern Calculation",
}

RESULTS_RE = re.compile(r"^(\w\S+)\s+(\S+) ns\s+(\S+) ns\s+(\d+)\s*(.*)$")
TRENDS = {
    "up": "🔼",
    "down": "🔽",
    "stable": "🔄",
}

def format_seconds(
    seconds: float,
    precision: int = 3,
    precision_over_minute: int = 2,
    force_hours: bool = False,
    force_minutes: bool = False,
) -> str:
    abs_seconds = abs(seconds)
    sign = ""
    if seconds < 0:
        sign = "-"
    hours = abs_seconds // 3600
    minutes = (abs_seconds % 3600) // 60
    seconds = abs_seconds % 60
    if precision_over_minute == 0:
        seconds_format = "2.0"
    else:
        seconds_format = f"{precision_over_minute + 3}.{precision_over_minute}"
    if hours >= 1 or force_hours:
        return f"{sign}{hours:.0f}:{minutes:02.0f}:{seconds % 60:0{seconds_format}f}"
    elif minutes >= 1 or force_minutes:
        return f"{sign}{minutes:.0f}:{seconds % 60:0{seconds_format}f}"
    else:
        return f"{sign}{seconds:.{precision}f}s"


def format_ns(ns: float, precision: int = 3, precision_over_minute: int = 2) -> str:
    abs_ns = abs(ns)
    if ns == 0:
        return "--"
    elif abs_ns < 10**3:
        return f"{ns:.{precision}f} ns"
    elif abs_ns < 10**6:
        return f"{ns / 10**3:.{precision}f} µs"
    elif abs_ns < 10**9:
        return f"{ns / 10**6:.{precision}f} ms"
    else:
        return format_seconds(
            ns / 10**9,
            precision=precision,
            precision_over_minute=precision_over_minute,
        )

class BenchmarkResult:
    def __init__(self, name: str = "", time: str = "0", cpu: str = "0", iterations: str = "0", details: str = ""):
        self.name = name
        self.time = float(time)
        self.cpu = float(cpu)
        self.iterations = int(iterations)
        self.details = details

    def __str__(self) -> str:
        return f"| {self.name} | {format_ns(self.time)} | {format_ns(self.cpu)} | {self.iterations} | {self.details} |"
    
    def __repr__(self) -> str:
        return f"BenchmarkResult({self.name}, {format_ns(self.time)})"
    
    def is_better_than(self, other: Self) -> bool:
        return self.time < other.time
    
    def is_worse_than(self, other: Self) -> bool:
        return self.time > other.time
    
    def difference_to(self, other: Self) -> float:
        return self.time - other.time
    
    def difference_ratio(self, other: Self) -> float:
        return (self.time - other.time) / other.time if other.time != 0 else 0

    def trend(self, other: Self) -> str:
        if self.time < other.time:
            return TRENDS["down"]
        elif self.time > other.time:
            return TRENDS["up"]
        else:
            return TRENDS["stable"]

class BenchmarkDiff:
    def __init__(self, *results: BenchmarkResult):
        self.results = results
    
    def __str__(self) -> str:
        result = f"| {self.results[0].name} |"
        for i, line in enumerate(self.results):
            result += f" {format_ns(line.time)} |"
            if i > 0:
                result += f" {line.trend(self.results[i - 1])} {line.difference_ratio(self.results[i - 1]):.2%} {format_ns(line.difference_to(self.results[i - 1]))} |"
        result += f" {self.details} |"
        return result
    
    def __repr__(self) -> str:
        return f"{self.max_difference:.2f} ({self.max_difference_ratio:.2%})"
    
    @property
    def details(self) -> str:
        """
        Return a string with the details of the benchmark. Find the first benchmark result with a non-empty details string.
        """
        for result in self.results:
            if result.details:
                return result.details
        return ""

    @property
    def max_difference_ratio(self) -> float:
        return max(result.difference_ratio(self.results[i - 1]) for i, result in enumerate(self.results) if i > 0)
    
    @property
    def max_difference(self) -> float:
        return max(result.difference_to(self.results[i - 1]) for i, result in enumerate(self.results) if i > 0)
    
    def is_significant(self, args: argparse.Namespace) -> bool:
        ratio = self.max_difference_ratio
        if ratio == 0:
            return False
        return ratio > args.diff_threshold_ratio or self.max_difference > args.diff_threshold_ns

class BenchmarkGroup:
    def __init__(self, name: str, benchmarks: list[BenchmarkResult]):
        self.name = name
        self.benchmarks = benchmarks

    def __str__(self) -> str:
        return f"## {self.name}\n\n{self.benchmarks}"
    
    def __repr__(self) -> str:
        return f"{self.name}, {self.benchmarks}"

class BenchmarkDiffGroup:
    def __init__(self, name: str, file_names: list[str], results: dict[str, BenchmarkGroup]):
        self.name = name
        self.file_names = file_names
        if not results:
            raise ValueError("Results cannot be empty")
        # now build benchmark diffs for each line
        # there should be one BenchmarkGroup per file in the results
        # if result is omitted, it means the benchmark was not run
        diffs: list[BenchmarkDiff] = []
        # first build a list of all the benchmark names
        benchmark_names: list[str] = []
        for file_name, group in results.items():
            for benchmark in group.benchmarks:
                if benchmark.name not in benchmark_names:
                    benchmark_names.append(benchmark.name)
        # now build a list of all the benchmark diffs
        for benchmark_name in benchmark_names:
            benchmark_results: list[BenchmarkResult] = []
            for file_name, group in results.items():
                benchmark_found = False
                for benchmark in group.benchmarks:
                    if benchmark.name == benchmark_name:
                        benchmark_results.append(benchmark)
                        benchmark_found = True
                        break
                if not benchmark_found:
                    benchmark_results.append(BenchmarkResult(benchmark_name))
            diffs.append(BenchmarkDiff(*benchmark_results))
        self.diffs = diffs
    
    def __repr__(self) -> str:
        return f"{self.name}, {self.max_difference_ratio:.2%} {format_ns(self.max_difference)}, {self.file_names}, {self.diffs}"
    
    @property
    def max_difference_ratio(self) -> float:
        return max(diff.max_difference_ratio for diff in self.diffs)
    
    @property
    def max_difference(self) -> float:
        return max(diff.max_difference for diff in self.diffs)
    
    def is_significant(self, args: argparse.Namespace) -> bool:
        return any(diff.is_significant(args) for diff in self.diffs)
    
    def table_header(self) -> str:
        return (f"| Benchmark | {self.file_names[0]} |" 
                + " | ".join(f"{file_name} | Δ" for file_name in self.file_names[1:]) + " | Details |\n" 
                + "| --- | --- |" + " | ".join("--- | ---" for _ in self.file_names[1:]) + " | --- |\n")
    
    def table_row(self, index: int) -> str:
        return str(self.diffs[index])
    
    def table_content(self, args: argparse.Namespace) -> str:
        return self.table_header() + "\n".join(f"{diff}" for diff in self.diffs if not args.significant_only or diff.is_significant(args))

    @classmethod
    def from_results(cls, results: dict[str, dict[str, BenchmarkGroup]]) -> list[Self]:
        """
        Build a list of BenchmarkDiffGroup for each file in the results
        results is a dict of file_name -> dict of group_name -> BenchmarkGroup
        """
        # first build a list of all group names
        group_names: list[str] = []
        for file_name, group in results.items():
            for group_name in group.keys():
                if group_name not in group_names:
                    group_names.append(group_name)
        # now build a list of BenchmarkDiffGroup for each group name
        diff_groups: list[Self] = []
        for group_name in group_names:
            group_results: dict[str, BenchmarkGroup] = {}
            for file_name, group in results.items():
                if group_name in group:
                    group_results[file_name] = group[group_name]
                else:
                    group_results[file_name] = BenchmarkGroup(group_name, [])
            diff_groups.append(cls(group_name, list(results.keys()), group_results))
        return diff_groups
        

def parse_results(file_name: str) -> str:
    with open(file_name, 'r') as file:
        content = file.read()

    groups: dict[str, BenchmarkGroup] = {}

    # Find all benchmark result lines
    for line in content.splitlines():
        match = RESULTS_RE.match(line)
        if match:
            benchmark_name = match.group(1)
            time = match.group(2)
            cpu = match.group(3)
            iterations = match.group(4)
            details = match.group(5)

            group_name = None
            for pattern, group_name in BENCHMARK_GROUPS.items():
                if pattern.match(benchmark_name):
                    break
            
            if group_name is None:
                print(f"Unknown benchmark: {benchmark_name}")
                group_name = "Other"
            print(f"Benchmark: {benchmark_name} -> {group_name}")
            if group_name not in groups:                
                groups[group_name] = BenchmarkGroup(group_name, [])
            groups[group_name].benchmarks.append(BenchmarkResult(benchmark_name, time, cpu, iterations, details))
    
    return groups

def print_results(args: argparse.Namespace, groups: dict[str, BenchmarkGroup]):
    content = ""
    for group_name, group in groups.items():
        header = "#" * args.group_header_level + " " + group_name
        content += header + "\n"

        table_content = TABLE_HEADER
        for benchmark in group.benchmarks:
            table_content += str(benchmark) + "\n"

        if args.wrap_in_details == "groups":
            table_content = DETAIL_BLOCK.format(content=table_content)

        content += table_content
        content += "\n"
    
    if args.wrap_in_details == "all":
        content = DETAIL_BLOCK.format(content=content)

    if args.output_file:
        with open(args.output_file, 'w') as file:
            file.write(content)
    else:
        print(content)

def print_diff(args: argparse.Namespace, diff_groups: list[BenchmarkDiffGroup]):
    content = ""
    for diff_group in diff_groups:
        if args.significant_only and not diff_group.is_significant(args):
            continue
        header = "#" * args.group_header_level + " " + diff_group.name
        content += f"\n{header}\n\n"
        table_content = diff_group.table_content(args)
        if args.wrap_in_details == "groups":
            table_content = DETAIL_BLOCK.format(content=table_content)
        content += table_content
        content += "\n"
    
    if args.wrap_in_details == "all":
        content = DETAIL_BLOCK.format(content=content)
    
    if args.output_file:
        with open(args.output_file, 'w') as file:
            file.write(content)
    else:
        print(content)

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='Format benchmark results')
    parser.add_argument('file_path', type=str, nargs='+', help='Path to the file to format')
    parser.add_argument('--group-header-level', "-g", type=int, default=2, help='Benchmark group header level')
    parser.add_argument('--wrap-in-details', "-w", type=str, choices=["none", "all", "groups"], default="none", help='Wrap all benchmarks in details, only groups or none')
    parser.add_argument('--diff-threshold-ratio', "-r", type=float, default=0.05, help='Threshold for difference to be considered significant')
    parser.add_argument('--diff-threshold-ns', "-n", type=float, default=10.0, help='Threshold for difference to be considered significant')
    parser.add_argument('--significant-only', "-s", action='store_true', help='Only show benchmarks that are significantly different')
    parser.add_argument('--output-file', "-o", type=str, help='Output file')

    return parser.parse_args()

def main():
    args = parse_args()
    results: dict[str, dict[str, BenchmarkGroup]] = {}
    for file_path in args.file_path:
        file_name = os.path.basename(file_path).split(".")[0]
        results[file_name] = parse_results(file_path)
    if len(results) == 1:
        # just print the results
        results = next(iter(results.values()))
        print_results(args, results)
    else:
        diff_groups = BenchmarkDiffGroup.from_results(results)
        print_diff(args, diff_groups)

if __name__ == '__main__':
    main()
