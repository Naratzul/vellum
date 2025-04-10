#ifdef WIN32
#include <Windows.h>
#include <lmcons.h>
#endif

#include "os.h"

namespace vellum {
namespace common {

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
#else
std::string getUserName() { return std::string(); }
std::string getComputerName() { return std::string(); }
#endif
}  // namespace common
}  // namespace vellum