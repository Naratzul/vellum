#include <cassert>
#include <cstdlib>
#include <string>
#ifdef _WIN32
#include <Windows.h>
#include <debugapi.h>
#include <lmcons.h>
#elif defined __APPLE__
#include <pwd.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <pwd.h>
#include <signal.h>
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

#else
std::string getUserName() {
  static std::string cached;
  static bool once = false;
  if (!once) {
    once = true;
    if (const passwd* pw = getpwuid(geteuid())) {
      if (pw->pw_name && pw->pw_name[0])
        cached = pw->pw_name;
    }
    if (cached.empty()) {
      if (const char* env = std::getenv("USER"))
        cached = env;
    }
  }
  return cached;
}

std::string getComputerName() {
  static std::string cached;
  static bool once = false;
  if (!once) {
    once = true;
    char buf[256];
    if (gethostname(buf, sizeof(buf)) == 0) {
      buf[sizeof(buf) - 1] = '\0';
      cached = buf;
    }
  }
  return cached;
}

#if defined __APPLE__
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
#else
void debugBreak() { raise(SIGTRAP); }
bool isDebuggerPresent() { return false; }
#endif
#endif
}  // namespace common
}  // namespace vellum
