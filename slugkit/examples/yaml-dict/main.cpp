#include <slugkit/generator/generator.hpp>
#include <slugkit/generator/structured_loader.hpp>

#include <userver/engine/run_standalone.hpp>
#include <userver/formats/yaml.hpp>

#include <boost/program_options.hpp>

#include <fstream>
#include <iostream>

#include <fmt/format.h>

int main(int argc, char* argv[]) try {
    namespace po = boost::program_options;
    po::options_description desc("YAML Dictionary Generator");
    // clang-format off
    desc.add_options()
        ("help,h", "produce help message")
        ("file,f", po::value<std::string>()->required(), "file to read")
        ("pattern,p", po::value<std::string>()->required(), "pattern to use")
        ("count,c", po::value<std::size_t>()->default_value(1), "number of slugs to generate")
        ("sequence,n", po::value<std::size_t>()->default_value(0), "sequence number")
        ("seed,s", po::value<std::string>(), "seed for the generator. If not provided, a random seed will be used")
    ;
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }
    po::notify(vm);

    auto file_name = vm["file"].as<std::string>();
    std::ifstream file(file_name);
    if (!file.is_open()) {
        throw std::runtime_error(fmt::format("Failed to open file: {}", file_name));
    }

    auto dictionary_set = slugkit::generator::DictionarySet::Parse<userver::formats::yaml::Value>(file);
    slugkit::generator::Generator generator(std::move(dictionary_set));

    auto pattern = vm["pattern"].as<std::string>();
    auto sequence = vm["sequence"].as<std::size_t>();
    std::string seed;
    if (vm.count("seed")) {
        seed = vm["seed"].as<std::string>();
    } else {
        seed = generator.RandomSeed();
    }
    auto count = vm["count"].as<std::size_t>();

    userver::engine::RunStandalone([&] {
        auto pattern_ptr = std::make_shared<slugkit::generator::Pattern>(pattern);
        std::cerr << "Pattern complexity: " << pattern_ptr->Complexity() << "\n---\n";
        // Run the generator in a standalone userver context
        if (count == 1) {
            std::cout << generator(pattern_ptr, seed, sequence) << '\n';
        } else {
            generator(pattern_ptr, seed, sequence, count, [](const std::string& slug) { std::cout << slug << '\n'; });
        }
    });

} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
}
