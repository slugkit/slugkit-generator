#pragma once

/// @file slugkit/compat/assert.hpp
/// @brief Compat shim for userver's UASSERT/UASSERT_MSG.

#ifdef SLUGKIT_USE_USERVER

#include <userver/utils/assert.hpp>

#else

#include <cassert>

#ifndef UASSERT
#define UASSERT(expr) assert(expr)
#endif

#ifndef UASSERT_MSG
#define UASSERT_MSG(expr, msg) assert((expr) && (msg))
#endif

#endif  // SLUGKIT_USE_USERVER
