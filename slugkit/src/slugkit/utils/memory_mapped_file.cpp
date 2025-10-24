#include <slugkit/utils/memory_mapped_file.hpp>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>
#include <system_error>

namespace slugkit::utils {

struct MemoryMappedFile::Impl {
    std::int32_t fd_ = -1;
    std::size_t size_ = 0;
    const std::byte* mapped_ = nullptr;

    Impl(const std::filesystem::path& filepath) {
        map(filepath);
    }

    Impl(const Impl& other) = delete;
    Impl& operator=(const Impl& other) = delete;

    Impl(Impl&& other) noexcept
        : fd_(other.fd_)
        , size_(other.size_)
        , mapped_(other.mapped_) {
        other.fd_ = -1;
        other.size_ = 0;
        other.mapped_ = nullptr;
    }

    Impl& operator=(Impl&& other) noexcept {
        if (this == &other) return *this;
        unmap();
        fd_ = other.fd_;
        size_ = other.size_;
        mapped_ = other.mapped_;
        other.fd_ = -1;
        other.size_ = 0;
        other.mapped_ = nullptr;
        return *this;
    }

    ~Impl() {
        unmap();
    }

    auto map(const std::filesystem::path& filepath) -> void {
        fd_ = ::open(filepath.c_str(), O_RDONLY);
        if (fd_ == -1) {
            throw std::runtime_error("Failed to open file: " + filepath.string());
        }

        struct stat st;
        if (fstat(fd_, &st) < 0) {
            ::close(fd_);
            throw std::system_error(errno, std::generic_category(), "Failed to stat file");
        }

        size_ = static_cast<std::size_t>(st.st_size);
        if (size_ == 0) {
            ::close(fd_);
            throw std::runtime_error("File is empty: " + filepath.string());
        }

        auto mapped = ::mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
        if (mapped == MAP_FAILED) {
            ::close(fd_);
            throw std::runtime_error("Failed to mmap file: " + filepath.string());
        }
        mapped_ = static_cast<std::byte*>(mapped);
    }

    auto unmap() noexcept -> void {
        if (mapped_ != nullptr && size_ > 0) {
            ::munmap(const_cast<std::byte*>(mapped_), size_);
            mapped_ = nullptr;
            size_ = 0;
        }
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    auto data() const noexcept -> DataType {
        return {mapped_, size_};
    }

    auto size() const noexcept -> std::size_t {
        return size_;
    }
};

MemoryMappedFile::MemoryMappedFile(const std::filesystem::path& filepath)
    : impl_(filepath) {
}

MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& other) noexcept
    : impl_(std::move(other.impl_)) {
}

MemoryMappedFile& MemoryMappedFile::operator=(MemoryMappedFile&& other) noexcept {
    if (this == &other) return *this;
    impl_ = std::move(other.impl_);
    return *this;
}

MemoryMappedFile::~MemoryMappedFile() = default;

auto MemoryMappedFile::data() const noexcept -> DataType {
    return impl_->data();
}

auto MemoryMappedFile::size() const noexcept -> std::size_t {
    return impl_->size();
}

}  // namespace slugkit::utils