// labm8_compat: stand-in for @labm8//labm8/cpp:string (+ strutil helpers).
#ifndef LABM8_COMPAT_LABM8_CPP_STRING_H_
#define LABM8_COMPAT_LABM8_CPP_STRING_H_

#include <cctype>
#include <string>

// labm8's string.h brings std::string into the GLOBAL namespace — the ProGraML
// sources use bare `string`. Replicate that so they compile unchanged.
using std::string;

namespace labm8 {

// Remove leading whitespace, in place.
inline void TrimLeft(std::string& s) {
  std::size_t i = 0;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
  s.erase(0, i);
}

inline bool HasSuffixString(const std::string& s, const std::string& suffix) {
  return s.size() >= suffix.size() &&
         s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Truncate to at most maxLength characters, appending "..." if truncated.
inline void TruncateWithEllipsis(std::string& s, std::size_t maxLength) {
  if (s.size() <= maxLength) return;
  const std::string ellipsis = "...";
  if (maxLength <= ellipsis.size()) {
    s = s.substr(0, maxLength);
  } else {
    s = s.substr(0, maxLength - ellipsis.size()) + ellipsis;
  }
}

}  // namespace labm8

#endif  // LABM8_COMPAT_LABM8_CPP_STRING_H_
