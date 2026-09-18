// labm8_compat: stand-in for @labm8//labm8/cpp:statusor.
#ifndef LABM8_COMPAT_LABM8_CPP_STATUSOR_H_
#define LABM8_COMPAT_LABM8_CPP_STATUSOR_H_

#include <cstdio>
#include <cstdlib>
#include <utility>

#include "labm8/cpp/status.h"

namespace labm8 {

// Holds either a value (ok) or an error Status. API subset used by the closure:
// construction from a value or a Status, .ok(), .status(), .ValueOrDie().
template <typename T>
class StatusOr {
 public:
  StatusOr(const T& value) : status_(), value_(value), has_value_(true) {}
  StatusOr(T&& value) : status_(), value_(std::move(value)), has_value_(true) {}
  StatusOr(const Status& status) : status_(status), value_(), has_value_(false) {}
  StatusOr(Status&& status) : status_(std::move(status)), value_(), has_value_(false) {}

  bool ok() const { return has_value_ && status_.ok(); }
  const Status& status() const { return status_; }

  // labm8 semantics: ValueOrDie() aborts if there is no value (turns a latent
  // "used the value of an error" bug into a loud crash instead of a silent
  // default-constructed value).
  const T& ValueOrDie() const& { CheckHasValue(); return value_; }
  T& ValueOrDie() & { CheckHasValue(); return value_; }
  T&& ValueOrDie() && { CheckHasValue(); return std::move(value_); }

 private:
  void CheckHasValue() const {
    if (!ok()) {
      std::fprintf(stderr, "labm8::StatusOr::ValueOrDie() on error status: %s\n",
                   status_.error_message().c_str());
      std::abort();
    }
  }

  Status status_;
  T value_;         // default-constructed in the error case (T must be default-ctible)
  bool has_value_;
};

}  // namespace labm8

#endif  // LABM8_COMPAT_LABM8_CPP_STATUSOR_H_
