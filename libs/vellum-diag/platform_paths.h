#pragma once

#include <string>
#include <string_view>

namespace vellum::diag {

std::string sentryDatabasePath(std::string_view appName);

#ifdef _WIN32
std::wstring sentryDatabasePathW(std::wstring_view appName);
#endif

}  // namespace vellum::diag
