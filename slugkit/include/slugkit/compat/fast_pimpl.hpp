#pragma once

/// @file slugkit/compat/fast_pimpl.hpp
/// @brief Compat shim for userver::utils::FastPimpl.
///
/// Standalone variant treats the declared Size/Align as an UPPER BOUND
/// (static_assert sizeof(T) <= Size) rather than userver's strict equality.
/// The existing kImplSize/kImplAlign constants in the public headers were tuned
/// for the userver/gcc+libstdc++ build; a different toolchain (clang/libc++,
/// NDK, iOS) can yield a different sizeof. Using an upper bound keeps the same
/// headers portable across toolchains while still catching genuine overflow.
/// If a constant turns out to be too small on a target, bump it in the header.

#ifdef SLUGKIT_USE_USERVER

#include <userver/utils/fast_pimpl.hpp>

namespace slugkit::compat {
using userver::utils::FastPimpl;
}  // namespace slugkit::compat

#else

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace slugkit::compat {

template <typename T, std::size_t Size, std::size_t Align>
class FastPimpl {
public:
    template <typename... Args>
    explicit FastPimpl(Args&&... args) noexcept(noexcept(T(std::declval<Args>()...))) {
        ::new (Storage()) T(std::forward<Args>(args)...);
    }

    FastPimpl(FastPimpl&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        ::new (Storage()) T(std::move(*other));
    }
    FastPimpl(const FastPimpl& other) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        ::new (Storage()) T(*other);
    }
    FastPimpl& operator=(FastPimpl&& other) noexcept(std::is_nothrow_move_assignable_v<T>) {
        **this = std::move(*other);
        return *this;
    }
    FastPimpl& operator=(const FastPimpl& other) noexcept(std::is_nothrow_copy_assignable_v<T>) {
        **this = *other;
        return *this;
    }

    ~FastPimpl() noexcept {
        Validate<sizeof(T), alignof(T)>();
        Storage()->~T();
    }

    T* operator->() noexcept { return Storage(); }
    const T* operator->() const noexcept { return Storage(); }
    T& operator*() noexcept { return *Storage(); }
    const T& operator*() const noexcept { return *Storage(); }
    T* get() noexcept { return Storage(); }
    const T* get() const noexcept { return Storage(); }

private:
    T* Storage() noexcept { return reinterpret_cast<T*>(&storage_); }
    const T* Storage() const noexcept { return reinterpret_cast<const T*>(&storage_); }

    template <std::size_t ActualSize, std::size_t ActualAlign>
    static void Validate() noexcept {
        static_assert(Size >= ActualSize, "FastPimpl Size is too small; bump the constant in the header");
        static_assert(Align >= ActualAlign, "FastPimpl Align is too small; bump the constant in the header");
        static_assert(Size % ActualAlign == 0 || Align % ActualAlign == 0, "FastPimpl alignment mismatch");
    }

    alignas(Align) std::byte storage_[Size];
};

}  // namespace slugkit::compat

#endif  // SLUGKIT_USE_USERVER
