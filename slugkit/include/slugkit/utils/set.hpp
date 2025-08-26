#pragma once

#include <algorithm>
#include <set>

namespace slugkit::utils {

// TODO concepts :)
template <typename ContainerT, typename ContainerU>
auto IsSubset(const ContainerT& subset, const ContainerU& superset) -> bool {
    for (const auto& item : subset) {
        if (!std::count(superset.begin(), superset.end(), item)) {
            return false;
        }
    }
    return true;
}

template <typename ContainerT, typename ContainerU>
auto Intersects(const ContainerT& lhs, const ContainerU& rhs) -> bool {
    if (lhs.empty() || rhs.empty()) {
        return false;
    }
    if (lhs.size() > rhs.size()) {
        return Intersects(rhs, lhs);
    }
    for (const auto& item : lhs) {
        if (std::count(rhs.begin(), rhs.end(), item)) {
            return true;
        }
    }
    return false;
}

}  // namespace slugkit::utils
