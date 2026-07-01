#pragma once

#include <string_view>

namespace vellum {

inline constexpr std::string_view kExpressionKeywords[] = {
    "if",       "else",     "while",    "for",      "break",
    "continue", "return",   "var",      "self",     "super",
    "true",     "false",    "none",
    "in",       "as",       "match",
};

inline constexpr std::string_view kTopLevelKeywords[] = {
    "script", "import",   "fun",    "event", "var",
    "state",  "property", "native", "static"
};

inline constexpr std::string_view kAllCompletionKeywords[] = {
    "if",       "else",     "while",    "for",      "break",
    "continue", "return",   "var",      "self",     "super",
    "true",     "false",    "none",     "native",   "static",
    "in",       "as",       "match",    "script",
    "import",   "fun",      "event",    "state",    "property",
};

}  // namespace vellum
