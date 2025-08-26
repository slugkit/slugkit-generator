#pragma once

#include <array>
#include <stdexcept>

namespace slugkit::utils {

template <typename Key, typename Value, std::size_t Capacity>
class SmallMap {
public:
    using value_type = std::pair<Key, Value>;
    using iterator = typename std::array<value_type, Capacity>::iterator;
    using const_iterator = typename std::array<value_type, Capacity>::const_iterator;

public:
    constexpr SmallMap() noexcept = default;
    constexpr SmallMap(std::initializer_list<value_type> init) noexcept
        : size_{init.size()} {
        std::copy(init.begin(), init.end(), data_.begin());
    }

    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t {
        return size_;
    }

    [[nodiscard]] constexpr auto empty() const noexcept -> bool {
        return size_ == 0;
    }

    constexpr auto clear() noexcept -> void {
        size_ = 0;
    }

    constexpr auto begin() noexcept -> iterator {
        return data_.begin();
    }

    constexpr auto begin() const noexcept -> const_iterator {
        return data_.begin();
    }

    constexpr auto end() noexcept -> iterator {
        return data_.begin() + size_;
    }

    constexpr auto end() const noexcept -> const_iterator {
        return data_.begin() + size_;
    }

    constexpr auto find(const Key& key) noexcept -> iterator {
        for (auto it = begin(); it != end(); ++it) {
            if (it->first == key) {
                return it;
            }
        }
        return end();
    }

    constexpr auto find(const Key& key) const noexcept -> const_iterator {
        for (auto it = begin(); it != end(); ++it) {
            if (it->first == key) {
                return it;
            }
        }
        return end();
    }

    constexpr auto contains(const Key& key) const noexcept -> bool {
        return find(key) != end();
    }

    constexpr auto at(const Key& key) -> Value& {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("Key not found");
        }
        return it->second;
    }

    constexpr auto at(const Key& key) const -> const Value& {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("Key not found");
        }
        return it->second;
    }

    constexpr auto operator[](const Key& key) -> Value& {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("Key not found");
        }
        return it->second;
    }

private:
    std::size_t size_ = 0;
    std::array<value_type, Capacity> data_;
};

}  // namespace slugkit::utils
