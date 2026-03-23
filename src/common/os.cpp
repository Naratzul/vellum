#include <cassert>
#include <cstdlib>
#include <filesystem>

#ifdef WIN32
#include <Windows.h>
#include <debugapi.h>
#include <lmcons.h>
#elif defined __APPLE__
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "os.h"

namespace vellum {
namespace common {

namespace fs = std::filesystem;

#ifdef WIN32
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

std::string getSentryDatabasePath(std::string_view appName) {
  wchar_t* localAppData = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr,
                                     &localAppData))) {
    std::string result =
        (fs::path(localAppData) / "Vellum" / appName / ".sentry-native")
            .string();
    CoTaskMemFree(localAppData);
    return result;
  }

  if (const wchar_t* userProfile = _wgetenv(L"USERPROFILE")) {
    return (fs::path(userProfile) / "AppData" / "Local" / "Vellum" / appName /
            ".sentry-native")
        .string();
  }

  return (fs::path(".") / ".sentry-native").string();
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
    return (fs::path(home) / "Library" / "Caches" / "Vellum" / appName /
            ".sentry-native")
        .string();
  }
  return (fs::path(".") / ".sentry-native").string();
}

#else
#error "Unsupported platform."
#endif
}  // namespace common
}  // namespace vellum