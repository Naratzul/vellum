#include <cxxopts.hpp>
#include <iostream>

#include "common/types.h"
#include "vellum/vellum.h"

int main(int argc, char *argv[]) {
  using vellum::common::Vec;

  cxxopts::Options options("vellum", "Vellum Compiler");
  options.add_options()("h,help", "Print help")("v,version", "Print version")(
      "f,file", "Input file", cxxopts::value<std::string>())(
      "i,import", "Import directory paths", cxxopts::value<Vec<std::string>>());

  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return EXIT_SUCCESS;
  }

  if (result.count("version")) {
    std::cout << "Vellum Compiler v0.1.0" << std::endl;
    return EXIT_SUCCESS;
  }

  if (!result.count("file")) {
    std::cerr << "No input file given." << std::endl;
    return EXIT_FAILURE;
  }

  Vec<std::string> importPaths;
  if (result.count("import")) {
    importPaths = result["import"].as<Vec<std::string>>();
    std::cout << "Import paths: ";
    for (const auto &path : importPaths) {
      std::cout << "- " << path << std::endl;
    }
  }

  const auto inputFile = result["file"].as<std::string>();
  std::cout << "Compiling " << inputFile << std::endl;

  try {
    vellum::Vellum().run(inputFile, importPaths);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}