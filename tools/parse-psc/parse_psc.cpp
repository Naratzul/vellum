#include <filesystem>
#include <iostream>
#include <vector>

#include "analyze/declaration_analysis.h"
#include "analyze/import_resolver.h"
#include "analyze/type_collector.h"
#include "common/fs.h"
#include "common/os.h"
#include "common/types.h"
#include "compiler/builtin_functions.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "parser/papyrus_lexer.h"
#include "parser/papyrus_parser.h"

namespace {
namespace fs = std::filesystem;

int run(const fs::path& inputPath) {
  using namespace vellum;

  using common::makeShared;
  using common::makeUnique;
  using common::pathToUtf8;

  std::string source;
  try {
    source = common::readFileContent(inputPath);
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }

  auto errorHandler = makeShared<CompilerErrorHandler>();
  vellum::PapyrusParser parser(makeUnique<PapyrusLexer>(source), errorHandler);
  auto result = parser.parse();

  if (errorHandler->hadError()) {
    errorHandler->printErrors();
    return 1;
  }

  std::cout << "Parsed " << pathToUtf8(inputPath) << ": "
            << result.declarations.size() << " top-level declaration(s)\n";

  auto filename = inputPath.stem().string();
  Vec<fs::path> importPaths = {fs::path(
      "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Skyrim Special "
      "Edition\\Data\\Source\\Scripts")};

  auto importLibrary = makeShared<ImportLibrary>(importPaths);
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier(filename)),
                           errorHandler, importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(result.declarations);

  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  collectDeclarations(result.declarations, errorHandler, resolver, filename);

  if (errorHandler->hadError()) {
    errorHandler->printErrors();
    return 1;
  }

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
