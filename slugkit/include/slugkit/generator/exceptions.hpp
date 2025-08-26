#pragma once

#include <stdexcept>

namespace slugkit::generator {

class GeneratorError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class PatternSyntaxError : public GeneratorError {
    using GeneratorError::GeneratorError;
};

class SlugFormatError : public GeneratorError {
    using GeneratorError::GeneratorError;
};

}  // namespace slugkit::generator
