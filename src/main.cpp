#include <iostream>

#include "vellum/vellum.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "No input file given." << std::endl;
    return EXIT_FAILURE;
  }

  try {
    const auto inputFile = std::string(argv[1]);
    vellum::Vellum().run(inputFile);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}