// labm8_compat: a self-contained stand-in for @labm8//labm8/cpp:status.
//
// Provides exactly the labm8 API the ProGraML llvm2graph closure uses, backed by
// std only, so the tested builder .cc files compile unchanged under CMake (no Bazel,
// no external labm8/fmt/glog). See LLVM21_MIGRATION_PLAN.md.
#ifndef LABM8_COMPAT_LABM8_CPP_STATUS_H_
#define LABM8_COMPAT_LABM8_CPP_STATUS_H_

#include <sstream>
#include <string>
#include <utility>

namespace labm8 {
namespace error {
// Unscoped enum inside namespace `error` so BOTH `error::Code::INVALID_ARGUMENT`
// and `error::INVALID_ARGUMENT` resolve (the closure uses both spellings).
enum Code {
  OK = 0,
  CANCELLED,
  UNKNOWN,
  INVALID_ARGUMENT,
  DEADLINE_EXCEEDED,
  NOT_FOUND,
  ALREADY_EXISTS,
  PERMISSION_DENIED,
  UNAUTHENTICATED,
  RESOURCE_EXHAUSTED,
  FAILED_PRECONDITION,
  ABORTED,
  OUT_OF_RANGE,
  UNIMPLEMENTED,
  INTERNAL,
  UNAVAILABLE,
  DATA_LOSS,
};
}  // namespace error

namespace labm8_detail {
// Minimal `{}`-style formatter (labm8/fmt style). Each "{}" is replaced by the
// next argument via operator<<; surplus "{}" or args are left/dropped harmlessly.
inline void FormatInto(std::ostringstream& os, const char* fmt) { os << fmt; }

template <typename T, typename... Rest>
void FormatInto(std::ostringstream& os, const char* fmt, T&& value, Rest&&... rest) {
  for (; *fmt; ++fmt) {
    if (fmt[0] == '{' && fmt[1] == '}') {
      os << std::forward<T>(value);
      FormatInto(os, fmt + 2, std::forward<Rest>(rest)...);
      return;
    }
    os << *fmt;
  }
}

template <typename... Args>
std::string Format(const std::string& fmt, Args&&... args) {
  std::ostringstream os;
  FormatInto(os, fmt.c_str(), std::forward<Args>(args)...);
  return os.str();
}
}  // namespace labm8_detail

class Status {
 public:
  Status() : code_(error::OK) {}

  // labm8-style: Status(code, "text with {} holes", args...). With no args the
  // message is used verbatim.
  template <typename... Args>
  Status(error::Code code, const std::string& fmt, Args&&... args)
      : code_(code),
        message_(labm8_detail::Format(fmt, std::forward<Args>(args)...)) {}

  bool ok() const { return code_ == error::OK; }
  error::Code code() const { return code_; }
  const std::string& error_message() const { return message_; }
  const std::string& message() const { return message_; }
  std::string ToString() const { return ok() ? std::string("OK") : message_; }

  // labm8 exposes a static OK constant used as `Status::OK` (no parens).
  static const Status OK;

 private:
  error::Code code_;
  std::string message_;
};

inline const Status Status::OK = Status();

}  // namespace labm8

#endif  // LABM8_COMPAT_LABM8_CPP_STATUS_H_
