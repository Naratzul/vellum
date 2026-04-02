#include "common/os.h"
#define CXXOPTS_VECTOR_DELIMITER ';'

#include <cstdlib>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>

#include "common/fs.h"
#include "common/sentry_report.h"
#include "common/types.h"
#include "vellum/vellum.h"

#define SENTRY_BUILD_STATIC 1
#include <sentry.h>

using vellum::common::Vec;

int main(int argc, char *argv[]) {
  cxxopts::Options options("vellum", "Vellum Compiler");
  options.add_options()("h,help", "Print help")("v,version", "Print version")(
      "f,file", "Input file", cxxopts::value<std::string>())(
      "i,import", "Import directory paths", cxxopts::value<Vec<std::string>>())(
      "g,debug-info", "Emit PEX debug info (source line mapping)",
      cxxopts::value<bool>()->default_value("true"))(
      "disable-crash-reporting", "Disable automatic crash reporting",
      cxxopts::value<bool>()->default_value("false"));

  auto result = options.parse(argc, argv);
  sentry_options_t *sentry_opts = nullptr;
  if (!result["disable-crash-reporting"].as<bool>()) {
    sentry_opts = sentry_options_new();
#ifdef VELLUM_SENTRY_DSN
    sentry_options_set_dsn(sentry_opts, VELLUM_SENTRY_DSN);
#else
    if (const char *dsn = std::getenv("SENTRY_DSN")) {
      sentry_options_set_dsn(sentry_opts, dsn);
    }
#endif
    sentry_options_set_release(sentry_opts, "vellum@0.1.0");
    sentry_options_set_database_path(
        sentry_opts, vellum::common::getSentryDatabasePath("vellum").c_str());
    const char *env = std::getenv("SENTRY_ENVIRONMENT");
    sentry_options_set_environment(sentry_opts,
                                   env && env[0] ? env : "development");
#ifndef NDEBUG
    sentry_options_set_debug(sentry_opts, 1);
#endif
    if (sentry_init(sentry_opts) == 0) {
      vellum::common::setSentryManualCaptureEnabled(true);
    }
  }

  struct SentryShutdown {
    ~SentryShutdown() {
      vellum::common::setSentryManualCaptureEnabled(false);
      sentry_close();
    }
  } sentryShutdown;

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

  importPaths.insert(importPaths.begin(),
                     std::filesystem::path(inputFile).parent_path().string());
  importPaths.insert(importPaths.begin(),
                     std::filesystem::current_path().string());

  importPaths = vellum::common::dedupePathsPreserveOrder(importPaths);

  const bool emitDebugInfo = result["debug-info"].as<bool>();
  std::cout << "Compiling " << inputFile << std::endl;

  bool runResult = false;
  try {
    runResult = vellum::Vellum().run(inputFile, importPaths, emitDebugInfo);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    vellum::common::captureSentryException(e, "main");
  } catch (...) {
    std::cerr << "Non-std exception" << std::endl;
    vellum::common::captureSentryUnknown("main");
  }

  return runResult ? EXIT_SUCCESS : EXIT_FAILURE;
}