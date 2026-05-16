#include "server.h"
#include "common/os.h"

int main() {
  using namespace vellum;

  /*while (!common::isDebuggerPresent()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }*/

  LspServer server;
  bool result = server.run();

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}