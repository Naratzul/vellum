#include "server.h"

int main() {
  using namespace vellum;

  LspServer server;
  bool result = server.run();

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}