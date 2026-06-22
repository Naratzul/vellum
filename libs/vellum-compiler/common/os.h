#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace vellum {
namespace common {

namespace fs = std::filesystem;

std::string pathToUtf8(const fs::path& p);
std::string unicodeToUtf8(const wchar_t* p);

std::string getUserName();
std::string getComputerName();

void debugBreak();
bool isDebuggerPresent();

}  // namespace common
}  // namespace vellum