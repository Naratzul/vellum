#pragma once

#include <string>

namespace vellum {
namespace common {

std::string getUserName();
std::string getComputerName();

void debugBreak();
bool isDebuggerPresent();

std::string getSentryDatabasePath(std::string_view appName);
}  // namespace common
}  // namespace vellum