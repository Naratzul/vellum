#include <cxxopts.hpp>
#include <iostream>

#include "vellum/vellum.h"

int main(int argc, char *argv[]) {
  cxxopts::Options options("vellum", "Vellum Compiler");
  options.add_options()("h,help", "Print help")("v,version", "Print version")(
      "i,input", "Input file", cxxopts::value<std::string>());

  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return EXIT_SUCCESS;
  }

  if (result.count("version")) {
    std::cout << "Vellum Compiler v0.1.0" << std::endl;
    return EXIT_SUCCESS;
  }

  if (!result.count("input")) {
    std::cerr << "No input file given." << std::endl;
    return EXIT_FAILURE;
  }

  const auto inputFile = result["input"].as<std::string>();
  std::cout << "Compiling " << inputFile << "..." << std::endl;

  try {
    vellum::Vellum().run(inputFile);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}