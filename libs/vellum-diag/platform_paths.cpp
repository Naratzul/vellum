#include "platform_paths.h"

#include <filesystem>
#include <string>

#ifdef _WIN32
#include <shlobj_core.h>
#elif defined __APPLE__
#include <cstdlib>
#else
#error "Unsupported platform."
#endif

namespace vellum::diag {
namespace {

namespace fs = std::filesystem;

std::string pathToUtf8(const fs::path& p) {
  const std::u8string u8 = p.u8string();
  return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

}  // namespace

#ifdef _WIN32
std::wstring sentryDatabasePathW(std::wstring_view appName) {
  wchar_t* localAppData = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr,
                                     &localAppData))) {
    std::wstring result =
        (fs::path(localAppData) / L"Vellum" / appName / L".sentry-native")
            .wstring();
    CoTaskMemFree(localAppData);
    return result;
  }

  if (const wchar_t* userProfile = _wgetenv(L"USERPROFILE")) {
    return (fs::path(userProfile) / L"AppData" / L"Local" / L"Vellum" /
            appName / L".sentry-native")
        .wstring();
  }

  return (fs::path(L".") / L".sentry-native").wstring();
}

std::string sentryDatabasePath(std::string_view appName) {
  return pathToUtf8(
      fs::path(sentryDatabasePathW(fs::path(appName).wstring())));
}

#elif defined __APPLE__
std::string sentryDatabasePath(std::string_view appName) {
  if (const char* home = getenv("HOME")) {
    return pathToUtf8(fs::path(home) / "Library" / "Caches" / "Vellum" /
                      appName / ".sentry-native");
  }
  return pathToUtf8(fs::path(".") / ".sentry-native");
}
#endif

}  // namespace vellum::diag
