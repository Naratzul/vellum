#pragma once

#include <string>

namespace vellum {
namespace common {

std::string getUserName();
std::string getComputerName();

void debugBreak();
bool isDebuggerPresent();
}  // namespace common
}  // namespace vellum