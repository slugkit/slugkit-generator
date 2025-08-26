#pragma once

#include <memory>

namespace slugkit::generator {

struct Pattern;
struct PatternSettings;
using PatternPtr = std::shared_ptr<Pattern>;
class PatternGenerator;
using PatternGeneratorPtr = std::shared_ptr<PatternGenerator>;

}  // namespace slugkit::generator
