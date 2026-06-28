#pragma once

#include <slugkit/compat/fast_pimpl.hpp>

#include <filesystem>
#include <span>

namespace slugkit::utils {

/// @brief Memory-mapped file
/// @details This file provides read-only access to the file contents.
class MemoryMappedFile {
public:
    using DataType = std::span<const std::byte>;
    using SizeType = DataType::size_type;

public:
    MemoryMappedFile(const std::filesystem::path& filepath);
    ~MemoryMappedFile();

    MemoryMappedFile(const MemoryMappedFile& other) = delete;
    MemoryMappedFile(MemoryMappedFile&& other) noexcept;

    auto operator=(const MemoryMappedFile& other) -> MemoryMappedFile& = delete;
    auto operator=(MemoryMappedFile&& other) noexcept -> MemoryMappedFile&;

    [[nodiscard]] auto data() const noexcept -> DataType;
    [[nodiscard]] auto size() const noexcept -> SizeType;

private:
    struct Impl;
    // Impl is identical across toolchains here, so a single size serves both builds.
    static constexpr std::size_t kPimplSize = 24UL;
    static constexpr std::size_t kPimplAlign = 8UL;
    slugkit::compat::FastPimpl<Impl, kPimplSize, kPimplAlign> impl_;
};

}  // namespace slugkit::utils
