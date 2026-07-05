#include "common/os.h"
#define CXXOPTS_VECTOR_DELIMITER ';'

#include <algorithm>
#include <cstdlib>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

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

bool isVelSourceFile(const fs::path& path) {
  const auto ext = path.extension().string();
  return ext == ".vel";
}

Vec<fs::path> discoverVelSources(const fs::path& directory, bool recursive) {
  if (!fs::exists(directory)) {
    throw std::runtime_error("Input directory does not exist: " +
                             vellum::common::pathToUtf8(directory));
  }
  if (!fs::is_directory(directory)) {
    throw std::runtime_error("Input path is not a directory: " +
                             vellum::common::pathToUtf8(directory));
  }

  Vec<fs::path> files;
  const auto collectFile = [&](const fs::directory_entry& entry) {
    if (!entry.is_regular_file()) {
      return;
    }
    if (!isVelSourceFile(entry.path())) {
      return;
    }
    files.push_back(fs::absolute(entry.path()));
  };

  if (recursive) {
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
      collectFile(entry);
    }
  } else {
    for (const auto& entry : fs::directory_iterator(directory)) {
      collectFile(entry);
    }
  }

  std::sort(files.begin(), files.end());
  return files;
}

void validateUniqueScriptNames(const Vec<fs::path>& files) {
  using vellum::common::pathToUtf8;

  std::unordered_map<std::string, fs::path> stems;
  stems.reserve(files.size() * 2);

  for (const auto& file : files) {
    const std::string stem = pathToUtf8(file.stem());
    const auto [it, inserted] = stems.emplace(stem, file);
    if (!inserted) {
      throw std::runtime_error("Duplicate script name '" + stem +
                               "' in batch: " + pathToUtf8(it->second) +
                               " and " + pathToUtf8(file));
    }
  }
}

Vec<fs::path> buildBaseImportPaths(const Vec<fs::path>& cliImportPaths) {
  Vec<fs::path> importPaths;
  importPaths.push_back(fs::current_path());
  importPaths.insert(importPaths.end(), cliImportPaths.begin(),
                     cliImportPaths.end());
  return vellum::common::dedupePathsPreserveOrder(importPaths);
}

Vec<fs::path> buildImportPathsForFile(const fs::path& inputFile,
                                      const Vec<fs::path>& baseImportPaths,
                                      const Opt<fs::path>& batchRoot) {
  Vec<fs::path> importPaths = baseImportPaths;
  if (batchRoot) {
    importPaths.push_back(*batchRoot);
  } else {
    importPaths.push_back(inputFile.parent_path());
  }
  return vellum::common::dedupePathsPreserveOrder(importPaths);
}

struct CompileOptions {
  Vec<fs::path> baseImportPaths;
  Opt<fs::path> batchRoot;
  bool emitDebugInfo;
  Opt<fs::path> outputDirectory;
};

void onStdException(const std::exception& e) {
  std::cerr << e.what() << std::endl;
  vellum::diag::captureException(e, "main");
}

void onUnknownException() {
  std::cerr << "Non-std exception" << std::endl;
  vellum::diag::captureUnknown("main");
}

bool compileOneFile(const fs::path& inputFile, const CompileOptions& options) {
  const Vec<fs::path> importPaths = buildImportPathsForFile(
      inputFile, options.baseImportPaths, options.batchRoot);

  return vellum::Vellum().run(inputFile, importPaths, options.emitDebugInfo,
                              options.outputDirectory);
}

int compileBatch(const Vec<fs::path>& files, const CompileOptions& options) {
  int succeeded = 0;
  int failed = 0;

  for (const auto& file : files) {
    std::cout << "Compiling " << file.relative_path() << " ... " << std::endl;

    bool ok = false;
    vellum::diag::runGuarded([&] { ok = compileOneFile(file, options); },
                             [](const std::exception& e) { onStdException(e); },
                             []() { onUnknownException(); });

    if (ok) {
      std::cout << "OK" << std::endl;
      ++succeeded;
    } else {
      std::cout << "FAILED" << std::endl;
      ++failed;
    }
  }

  std::cout << "---" << std::endl;
  std::cout << "Compiled " << succeeded << "/" << files.size();
  if (failed > 0) {
    std::cout << " (" << failed << " failed)";
  }
  std::cout << std::endl;

  return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
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
      "f,file", "Input .vel file or directory of sources",
      cxxopts::value<fs::path>())("i,import", "Import directory paths",
                                  cxxopts::value<Vec<fs::path>>())(
      "o,output", "Output directory for the compiled .pex file",
      cxxopts::value<fs::path>())(
      "no-recursive",
      "When -f is a directory, only compile sources in that directory (not "
      "subdirectories)",
      cxxopts::value<bool>()->default_value("false"))(
      "r,release",
      "Omit PEX source line mapping (default: emit line mapping like Papyrus)",
      cxxopts::value<bool>()->default_value("false"))(
      "disable-crash-reporting", "Disable automatic crash reporting",
      cxxopts::value<bool>()->default_value("false"));

  auto result = options.parse(argc, argv);
  const vellum::diag::SentrySession sentrySession(
      crashReportingOptions(!result["disable-crash-reporting"].as<bool>()));

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return EXIT_SUCCESS;
  }

  if (result.count("version")) {
    std::cout << "Vellum Compiler v" << VELLUM_VERSION << std::endl;
    return EXIT_SUCCESS;
  }

  if (!result.count("file")) {
    std::cerr << "No input file or directory given." << std::endl;
    return EXIT_FAILURE;
  }

  Vec<fs::path> cliImportPaths;
  if (result.count("import")) {
    cliImportPaths = result["import"].as<Vec<fs::path>>();

    for (auto& path : cliImportPaths) {
      path = sanitizeCliPath(path);
    }

    std::cout << "Import paths: ";
    for (const auto& path : cliImportPaths) {
      std::cout << "- " << path << std::endl;
    }
  }

  const auto inputPath = sanitizeCliPath(result["file"].as<fs::path>());
  const bool recursive = !result["no-recursive"].as<bool>();
  const bool emitDebugInfo = !result["release"].as<bool>();

  Opt<fs::path> outputDirectory;
  if (result.count("output")) {
    outputDirectory = sanitizeCliPath(result["output"].as<fs::path>());
    std::cout << "Output directory: " << *outputDirectory << std::endl;
  }

  const CompileOptions compileOptions{
      .baseImportPaths = buildBaseImportPaths(cliImportPaths),
      .batchRoot = std::nullopt,
      .emitDebugInfo = emitDebugInfo,
      .outputDirectory = outputDirectory,
  };

  int exitCode = EXIT_FAILURE;
  vellum::diag::runGuarded(
      [&] {
        if (fs::is_directory(inputPath)) {
          std::cout << "Scanning " << inputPath.relative_path() << " ("
                    << (recursive ? "recursive" : "non-recursive") << ") ..."
                    << std::endl;

          Vec<fs::path> files = discoverVelSources(inputPath, recursive);
          if (files.empty()) {
            std::cerr << "No .vel or .vellum files found in "
                      << inputPath.relative_path() << std::endl;
            return;
          }

          validateUniqueScriptNames(files);
          std::cout << "Found " << files.size() << " source file"
                    << (files.size() == 1 ? "" : "s") << std::endl;

          CompileOptions batchOptions = compileOptions;
          batchOptions.batchRoot = fs::absolute(inputPath);
          exitCode = compileBatch(files, batchOptions);
          return;
        }

        if (!fs::is_regular_file(inputPath)) {
          std::cerr << "Input path is not a file or directory: " << inputPath
                    << std::endl;
          return;
        }

        if (!isVelSourceFile(inputPath)) {
          std::cerr << "Input file must have a .vel extension: " << inputPath
                    << std::endl;
          return;
        }

        std::cout << "Compiling " << inputPath.relative_path() << std::endl;
        exitCode = compileOneFile(inputPath, compileOptions) ? EXIT_SUCCESS
                                                             : EXIT_FAILURE;
      },
      [](const std::exception& e) { onStdException(e); },
      []() { onUnknownException(); });

  return exitCode;
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
