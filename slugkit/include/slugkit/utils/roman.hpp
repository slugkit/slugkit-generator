#pragma once

#include <string>
#include <string_view>

namespace slugkit::utils::roman {

std::string ToRoman(int num, bool lower = false);
int ParseRoman(std::string_view roman);

}  // namespace slugkit::utils::roman
