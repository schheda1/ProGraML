// labm8_compat: stand-in for @labm8//labm8/cpp:logging (glog-style CHECK/LOG).
//
// Supports the streamed form used by the closure: `CHECK(x) << "msg"`,
// `DCHECK(x) << "msg"`, `LOG(FATAL) << "msg"`.
#ifndef LABM8_COMPAT_LABM8_CPP_LOGGING_H_
#define LABM8_COMPAT_LABM8_CPP_LOGGING_H_

#include <cstdlib>
#include <iostream>
#include <sstream>

namespace labm8_log {

// Accumulates a message and, on destruction, flushes to stderr; aborts if fatal.
class LogMessage {
 public:
  explicit LogMessage(bool fatal) : fatal_(fatal) {}
  ~LogMessage() {
    std::cerr << os_.str() << std::endl;
    if (fatal_) std::abort();
  }
  std::ostream& stream() { return os_; }

 private:
  std::ostringstream os_;
  bool fatal_;
};

// Swallows a stream so a disabled DCHECK still type-checks `<< msg` at zero cost.
class NullStream {
 public:
  template <typename T>
  NullStream& operator<<(const T&) { return *this; }
};

// `Voidify() & (ostream << ...)` coerces the streamed expression to void so it can
// sit in the ternary below. `&` binds looser than `<<`, so the whole chain streams
// first, then the LogMessage temporary's destructor runs (and aborts) at ';'.
struct Voidify {
  void operator&(std::ostream&) {}
};

}  // namespace labm8_log

#define LABM8_LOG_FATAL ::labm8_log::LogMessage(true).stream()
#define LABM8_LOG_ERROR ::labm8_log::LogMessage(false).stream()
#define LABM8_LOG_WARNING ::labm8_log::LogMessage(false).stream()
#define LABM8_LOG_INFO ::labm8_log::LogMessage(false).stream()
#define LOG(severity) LABM8_LOG_##severity

#define CHECK(condition)                                       \
  (condition) ? (void)0                                        \
              : ::labm8_log::Voidify() &                       \
                    ::labm8_log::LogMessage(true).stream()     \
                        << "Check failed: " #condition " "

#ifdef NDEBUG
#define DCHECK(condition) \
  while (false) ::labm8_log::NullStream()
#else
#define DCHECK(condition) CHECK(condition)
#endif

#endif  // LABM8_COMPAT_LABM8_CPP_LOGGING_H_
