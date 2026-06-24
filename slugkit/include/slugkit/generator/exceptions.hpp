#pragma once

#include <stdexcept>

namespace slugkit::generator {

/// @brief Base class for all generator errors.
class GeneratorError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// @brief Error thrown when a pattern syntax is invalid.
class PatternSyntaxError : public GeneratorError {
    using GeneratorError::GeneratorError;
};

/// @brief Error thrown when a dictionary operation fails.
class DictionaryError : public GeneratorError {
    using GeneratorError::GeneratorError;
};

/// @brief Error thrown when a dictionary data is invalid.
class DictionaryDataError : public DictionaryError {
    using DictionaryError::DictionaryError;
};

/// @brief Error thrown when a dictionary filter operation fails.
class DictionaryFilterError : public DictionaryError {
    using DictionaryError::DictionaryError;
};

/// @brief Error thrown when a slug format is invalid.
class SlugFormatError : public GeneratorError {
    using GeneratorError::GeneratorError;
};

}  // namespace slugkit::generator
