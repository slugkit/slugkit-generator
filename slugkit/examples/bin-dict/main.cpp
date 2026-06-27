#include <slugkit/generator/binary_dictionary.hpp>
#include <slugkit/generator/generator.hpp>

#include <userver/engine/run_standalone.hpp>

#include <boost/program_options.hpp>

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#include <fmt/format.h>

// CLI for generating from compiled binary (.bin) dictionaries -- the same code path the gen
// service uses with dictionary-source: files. Lets us reproduce / verify binary-path behaviour
// and timing (e.g. heavy tag filters over large dictionaries) locally before deploying.

namespace {

std::shared_ptr<std::vector<std::byte>> ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error(fmt::format("Failed to open file: {}", path));
    }
    const auto size = static_cast<std::size_t>(file.tellg());
    file.seekg(0);
    auto buffer = std::make_shared<std::vector<std::byte>>(size);
    file.read(reinterpret_cast<char*>(buffer->data()), static_cast<std::streamsize>(size));
    return buffer;
}

}  // namespace

int main(int argc, char* argv[]) try {
    namespace po = boost::program_options;
    po::options_description desc("Binary Dictionary Generator");
    // clang-format off
    desc.add_options()
        ("help,h", "produce help message")
        ("bin,b", po::value<std::vector<std::string>>()->required()->multitoken(),
            "one or more compiled .bin dictionary files to load")
        ("pattern,p", po::value<std::string>()->required(), "pattern to use")
        ("count,c", po::value<std::size_t>()->default_value(1), "number of slugs to generate")
        ("sequence,n", po::value<std::size_t>()->default_value(0), "sequence number")
        ("seed,s", po::value<std::string>(), "seed for the generator (random if omitted)")
    ;
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }
    po::notify(vm);

    auto bin_files = vm["bin"].as<std::vector<std::string>>();
    auto pattern = vm["pattern"].as<std::string>();
    auto sequence = vm["sequence"].as<std::size_t>();
    auto count = vm["count"].as<std::size_t>();

    // The generator uses userver synchronisation primitives, so build and run it inside a
    // standalone coroutine context.
    userver::engine::RunStandalone([&] {
        slugkit::generator::binary::DictionarySet dictionaries;
        for (const auto& path : bin_files) {
            auto buffer = ReadFile(path);
            std::span<const std::byte> data{buffer->data(), buffer->size()};
            dictionaries.Add(data, buffer);  // buffer kept alive by the DictionarySet
            std::cerr << fmt::format("Loaded {} ({} bytes)\n", path, buffer->size());
        }
        slugkit::generator::Generator generator(std::move(dictionaries));
        std::string seed = vm.count("seed") ? vm["seed"].as<std::string>() : generator.RandomSeed();

        auto pattern_ptr = std::make_shared<slugkit::generator::Pattern>(pattern);
        std::cerr << fmt::format("Pattern: {}\nComplexity: {}\nSeed: {}\n---\n", pattern,
                                 pattern_ptr->Complexity(), seed);

        const auto start = std::chrono::steady_clock::now();
        if (count == 1) {
            std::cout << generator(pattern_ptr, seed, sequence) << '\n';
        } else {
            generator(pattern_ptr, seed, sequence, count,
                      [](const std::string& slug) { std::cout << slug << '\n'; });
        }
        const auto elapsed = std::chrono::steady_clock::now() - start;
        const auto ms = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() / 1000.0;
        std::cerr << fmt::format("---\nGenerated {} slug(s) in {:.3f} ms ({:.3f} ms/slug)\n",
                                 count, ms, ms / static_cast<double>(count));
    });

} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
}
