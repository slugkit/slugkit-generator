#pragma once

/// @file slugkit/compat/shared_mutex.hpp
/// @brief Compat shim for a shared mutex.
///
/// IMPORTANT: under userver, std::shared_mutex must NOT be used — it blocks the
/// OS thread instead of yielding the coroutine. So the userver build aliases the
/// coroutine-aware engine::SharedMutex, and only the standalone build uses
/// std::shared_mutex. std::shared_lock / std::unique_lock work with both.

#ifdef SLUGKIT_USE_USERVER

#include <userver/engine/shared_mutex.hpp>

namespace slugkit::compat {
using SharedMutex = userver::engine::SharedMutex;
}  // namespace slugkit::compat

#else

#include <shared_mutex>

namespace slugkit::compat {
using SharedMutex = std::shared_mutex;
}  // namespace slugkit::compat

#endif  // SLUGKIT_USE_USERVER
