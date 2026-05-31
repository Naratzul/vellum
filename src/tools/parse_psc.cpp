#include <filesystem>
#include <iostream>
#include <vector>

#include "common/fs.h"
#include "common/os.h"
#include "common/types.h"
#include "compiler/compiler_error_handler.h"
#include "parser/papyrus_lexer.h"
#include "parser/papyrus_parser.h"

namespace {
namespace fs = std::filesystem;

int run(const fs::path& inputPath) {
  using vellum::common::makeShared;
  using vellum::common::makeUnique;
  using vellum::common::pathToUtf8;

  std::string source;
  try {
    source = vellum::common::readFileContent(inputPath);
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }

  auto errorHandler = makeShared<vellum::CompilerErrorHandler>();
  vellum::PapyrusParser parser(
      makeUnique<vellum::PapyrusLexer>(source), errorHandler);
  const auto result = parser.parse();

  if (errorHandler->hadError()) {
    errorHandler->printErrors();
    return 1;
  }

  std::cout << "Parsed " << pathToUtf8(inputPath) << ": "
            << result.declarations.size() << " top-level declaration(s)\n";
  return 0;
}

int entryPoint(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: parse_psc <file.psc>\n";
    return 2;
  }

  return run(fs::path(argv[1]));
}
}  // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
  std::vector<std::string> argsUtf8;
  argsUtf8.reserve(argc);
  for (int i = 0; i < argc; ++i) {
    argsUtf8.push_back(argv[i] ? vellum::common::unicodeToUtf8(argv[i])
                               : std::string{});
  }

  std::vector<char*> argsUtf8Ptr;
  argsUtf8Ptr.reserve(argsUtf8.size());
  for (auto& str : argsUtf8) {
    argsUtf8Ptr.push_back(str.data());
  }

  return entryPoint(static_cast<int>(argsUtf8Ptr.size()), argsUtf8Ptr.data());
}
#else
int main(int argc, char* argv[]) { return entryPoint(argc, argv); }
#endif
