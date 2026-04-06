#include <cassert>
#include <cstdlib>
#include <string>
#ifdef _WIN32
#include <Windows.h>
#include <debugapi.h>
#include <lmcons.h>
#include <shlobj_core.h>
#elif defined __APPLE__
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "os.h"

namespace vellum {
namespace common {

std::string pathToUtf8(const fs::path& p) {
  const std::u8string u8 = p.u8string();
  return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

std::string unicodeToUtf8(const wchar_t* p) { return pathToUtf8(fs::path(p)); }

#ifdef _WIN32
std::string getUserName() {
  static char buf[UNLEN + 1] = "";
  if (buf[0] == 0) {
    DWORD l = _countof(buf);
    GetUserName(buf, &l);
  }
  return buf;
}

std::string getComputerName() {
  static char buf[64] = "";
  if (buf[0] == 0) {
    DWORD l = _countof(buf);
    GetComputerName(buf, &l);
  }
  return buf;
}
void debugBreak() { DebugBreak(); }
bool isDebuggerPresent() { return IsDebuggerPresent(); }

std::wstring getSentryDatabasePathW(std::wstring_view appName) {
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

std::string getSentryDatabasePath(std::string_view appName) {
  return pathToUtf8(
      fs::path(getSentryDatabasePathW(fs::path(appName).wstring())));
}

#elif defined __APPLE__
std::string getUserName() { return std::string(); }
std::string getComputerName() { return std::string(); }

void debugBreak() { __builtin_trap(); }
bool isDebuggerPresent() {
  int junk;
  int mib[4];
  struct kinfo_proc info;
  size_t size;

  // Initialize the flags so that, if sysctl fails for some bizarre
  // reason, we get a predictable result.

  info.kp_proc.p_flag = 0;

  // Initialize mib, which tells sysctl the info we want, in this case
  // we're looking for information about a specific process ID.

  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PID;
  mib[3] = getpid();

  // Call sysctl.

  size = sizeof(info);
  junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
  assert(junk == 0);

  // We're being debugged if the P_TRACED flag is set.

  return ((info.kp_proc.p_flag & P_TRACED) != 0);
}

std::string getSentryDatabasePath(std::string_view appName) {
  if (const char* home = getenv("HOME")) {
    return pathToUtf8(fs::path(home) / "Library" / "Caches" / "Vellum" /
                      appName / ".sentry-native");
  }
  return pathToUtf8(fs::path(".") / ".sentry-native");
}

#else
#error "Unsupported platform."
#endif
}  // namespace common
}  // namespace vellum