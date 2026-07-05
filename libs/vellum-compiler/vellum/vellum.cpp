#include "vellum.h"

#include <ctime>

#include "analyze/import_library.h"
#include "analyze/import_resolver.h"
#include "analyze/semantic_analyzer.h"
#include "analyze/type_collector.h"
#include "common/fs.h"
#include "common/os.h"
#include "common/types.h"
#include "compiler/builtin_functions.h"
#include "compiler/compiler.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "pex/pex_file.h"
#include "vellum/vellum_literal.h"

namespace vellum {
using common::makeShared;
using common::makeUnique;
using common::Opt;
using common::Unique;

bool Vellum::run(const fs::path& inputFile,
                 const Vec<fs::path>& importPaths,
                 bool emitDebugInfo,
                 Opt<fs::path> outputDirectory) {
  const std::string& sourceCode = common::readFileContent(inputFile);
  const auto filename = common::pathToUtf8(inputFile.stem());

  auto lexer = makeUnique<Lexer>(sourceCode);
  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(importPaths);
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier(filename)),
                           errorHandler, importLibrary, builtinFunctions);

  Parser parser(std::move(lexer), errorHandler);
  ParserResult parseResult = parser.parse();

  if (errorHandler->hadError()) {
    errorHandler->printErrors();
    return false;
  }

  TypeCollector typeCollector;
  typeCollector.collect(parseResult.declarations);

  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes(),
                                   VellumIdentifier(filename));

  SemanticAnalyzer semantic(errorHandler, resolver, filename);
  const SemanticAnalyzeResult semanticResult =
      semantic.analyze(std::move(parseResult.declarations));
  if (errorHandler->hadError()) {
    errorHandler->printErrors();
    return false;
  }

  ScriptMetadata metadata;
  metadata.gameID = game::GameID::Skyrim;
  metadata.compilationTime = std::time(nullptr);
  metadata.sourceFile = common::pathToUtf8(fs::canonical(inputFile));
  metadata.userName = common::getUserName();
  metadata.computerName = common::getComputerName();
  metadata.emitDebugInfo = emitDebugInfo;

  Compiler compiler(errorHandler);
  pex::PexFile pexFile =
      compiler.compile(metadata, semanticResult.declarations);

  if (errorHandler->hadError()) {
    errorHandler->printErrors();
    return false;
  }

  fs::path outputFile;
  if (outputDirectory) {
    std::error_code ec;
    fs::create_directories(*outputDirectory, ec);
    if (ec) {
      throw std::runtime_error("Failed to create output directory: " +
                               common::pathToUtf8(*outputDirectory) + " (" +
                               ec.message() + ")");
    }
    outputFile = *outputDirectory / (inputFile.stem().string() + ".pex");
  } else {
    outputFile = inputFile;
    outputFile.replace_extension(fs::path(".pex"));
  }

  pexFile.writeToFile(outputFile);

  return true;
}
}  // namespace vellum