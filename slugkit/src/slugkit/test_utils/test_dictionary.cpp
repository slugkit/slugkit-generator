#include "test_dictionary.hpp"

#include <generated/test-adv.slugs.hpp>

namespace slugkit::generator::test {

const std::span<const std::byte> kDictionaryTestData = std::span<const std::byte>(
    reinterpret_cast<const std::byte*>(dictionary_test_data_begin),
    dictionary_test_data_size
);

}  // namespace slugkit::generator::test
