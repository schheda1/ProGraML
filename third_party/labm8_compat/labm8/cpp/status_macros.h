// labm8_compat: stand-in for @labm8//labm8/cpp:status_macros.
#ifndef LABM8_COMPAT_LABM8_CPP_STATUS_MACROS_H_
#define LABM8_COMPAT_LABM8_CPP_STATUS_MACROS_H_

#include <utility>

#include "labm8/cpp/status.h"

#define LABM8_MACRO_CONCAT_(a, b) a##b
#define LABM8_MACRO_CONCAT(a, b) LABM8_MACRO_CONCAT_(a, b)

// RETURN_IF_ERROR(expr): evaluate a Status; return it on error.
#define RETURN_IF_ERROR(expr)                                       \
  do {                                                              \
    ::labm8::Status LABM8_MACRO_CONCAT(_status_, __LINE__) = (expr); \
    if (!LABM8_MACRO_CONCAT(_status_, __LINE__).ok())              \
      return LABM8_MACRO_CONCAT(_status_, __LINE__);               \
  } while (0)

// ASSIGN_OR_RETURN(lhs, expr): evaluate a StatusOr; return its status on error,
// else assign the value to `lhs` (an already-declared lvalue in all call sites).
// Multi-statement (not do/while) because the assignment must persist; used at
// statement scope only, matching the closure's usage.
#define ASSIGN_OR_RETURN(lhs, expr)                                        \
  auto LABM8_MACRO_CONCAT(_statusor_, __LINE__) = (expr);                  \
  if (!LABM8_MACRO_CONCAT(_statusor_, __LINE__).ok())                      \
    return LABM8_MACRO_CONCAT(_statusor_, __LINE__).status();             \
  lhs = std::move(LABM8_MACRO_CONCAT(_statusor_, __LINE__).ValueOrDie())

#endif  // LABM8_COMPAT_LABM8_CPP_STATUS_MACROS_H_
