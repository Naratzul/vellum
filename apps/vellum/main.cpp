#include "common/os.h"
#define CXXOPTS_VECTOR_DELIMITER ';'

#include <cstdlib>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>

#include "common/fs.h"
#include "common/string_utils.h"
#include "common/types.h"
#include "run_guarded.h"
#include "sentry_report.h"
#include "sentry_session.h"
#include "vellum/vellum.h"

#ifndef VELLUM_VERSION
#error VELLUM_VERSION must be defined by CMake (project VERSION)
#endif
#ifndef VELLUM_SENTRY_RELEASE
#error VELLUM_SENTRY_RELEASE must be defined by CMake
#endif

using vellum::common::Opt;
using vellum::common::Vec;

namespace {
namespace fs = std::filesystem;

fs::path sanitizeCliPath(const fs::path& path) {
  using namespace vellum::common;

  auto raw = pathToUtf8(path);
  trim(raw);

  while (!raw.empty() && (raw.back() == '\'' || raw.back() == '"')) {
    raw.pop_back();
  }

  return fs::path(raw);
}

vellum::diag::CrashReportingOptions crashReportingOptions(bool enabled) {
  vellum::diag::CrashReportingOptions options{
      .appName = "vellum",
      .release = VELLUM_SENTRY_RELEASE,
      .enabled = enabled,
  };
#ifdef VELLUM_SENTRY_DSN
  options.dsn = VELLUM_SENTRY_DSN;
#endif
#ifdef VELLUM_SENTRY_ENVIRONMENT
  options.environment = VELLUM_SENTRY_ENVIRONMENT;
#endif
  return options;
}

int entryPoint(int argc, char* argv[]) {
  cxxopts::Options options("vellum", "Vellum Compiler");
  options.add_options()("h,help", "Print help")("v,version", "Print version")(
      "f,file", "Input file", cxxopts::value<fs::path>())(
      "i,import", "Import directory paths", cxxopts::value<Vec<fs::path>>())(
      "o,output", "Output directory for the compiled .pex file",
      cxxopts::value<fs::path>())(
      "r,release",
      "Omit PEX source line mapping (default: emit line mapping like Papyrus)",
      cxxopts::value<bool>()->default_value("false"))(
      "disable-crash-reporting", "Disable automatic crash reporting",
      cxxopts::value<bool>()->default_value("false"));

  auto result = options.parse(argc, argv);
  const vellum::diag::SentrySession sentrySession(crashReportingOptions(
      !result["disable-crash-reporting"].as<bool>()));

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return EXIT_SUCCESS;
  }

  if (result.count("version")) {
    std::cout << "Vellum Compiler v" << VELLUM_VERSION << std::endl;
    return EXIT_SUCCESS;
  }

  if (!result.count("file")) {
    std::cerr << "No input file given." << std::endl;
    return EXIT_FAILURE;
  }

  Vec<fs::path> importPaths;
  if (result.count("import")) {
    importPaths = result["import"].as<Vec<fs::path>>();

    for (auto& path : importPaths) {
      path = sanitizeCliPath(path);
    }

    std::cout << "Import paths: ";
    for (const auto& path : importPaths) {
      std::cout << "- " << path << std::endl;
    }
  }

  const auto inputFile = sanitizeCliPath(result["file"].as<fs::path>());

  importPaths.insert(importPaths.begin(), inputFile.parent_path());
  importPaths.insert(importPaths.begin(), fs::current_path());

  importPaths = vellum::common::dedupePathsPreserveOrder(importPaths);

  const bool emitDebugInfo = !result["release"].as<bool>();

  Opt<fs::path> outputDirectory;
  if (result.count("output")) {
    outputDirectory = sanitizeCliPath(result["output"].as<fs::path>());
    std::cout << "Output directory: " << *outputDirectory << std::endl;
  }

  std::cout << "Compiling " << inputFile.relative_path() << std::endl;

  bool runResult = false;

  vellum::diag::runGuarded(
      [&] {
        runResult = vellum::Vellum().run(inputFile, importPaths, emitDebugInfo,
                                         outputDirectory);
      },
      [&](const std::exception& e) {
        std::cerr << e.what() << std::endl;
        vellum::diag::captureException(e, "main");
      },
      [&] {
        std::cerr << "Non-std exception" << std::endl;
        vellum::diag::captureUnknown("main");
      });

  return runResult ? EXIT_SUCCESS : EXIT_FAILURE;
}
}  // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
  Vec<std::string> argsUtf8;
  argsUtf8.reserve(argc);

  for (int i = 0; i < argc; ++i) {
    if (argv[i]) {
      argsUtf8.push_back(vellum::common::unicodeToUtf8(argv[i]));
    } else {
      argsUtf8.emplace_back();
    }
  }

  Vec<char*> argsUtf8Ptr;
  argsUtf8Ptr.reserve(argsUtf8.size());

  for (auto& str : argsUtf8) {
    argsUtf8Ptr.push_back(str.data());
  }

  return entryPoint(argsUtf8Ptr.size(), argsUtf8Ptr.data());
}
#else
int main(int argc, char* argv[]) { return entryPoint(argc, argv); }
#endif
