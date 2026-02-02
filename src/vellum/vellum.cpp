#include "vellum.h"

#include <ctime>

#include "analyze/declaration_collector.h"
#include "analyze/import_library.h"
#include "analyze/import_resolver.h"
#include "analyze/semantic_analyzer.h"
#include "analyze/type_collector.h"
#include "common/fs.h"
#include "common/os.h"
#include "common/types.h"
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
using common::Unique;

void Vellum::run(const fs::path& inputFile,
                 const Vec<std::string>& importPaths) {
  const std::string& sourceCode = common::readFileContent(inputFile);
  const auto filename = inputFile.stem().string();

  auto lexer = makeUnique<Lexer>(sourceCode);
  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(importPaths);
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier(filename)),
                           errorHandler, importLibrary);

  Parser parser(std::move(lexer), errorHandler);
  ParserResult parseResult = parser.parse();

  if (errorHandler->hadError()) {
    return;
  }

  TypeCollector typeCollector;
  typeCollector.collect(parseResult.declarations);

  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  DeclarationCollector collector(errorHandler, resolver, filename);
  collector.collect(parseResult.declarations);

  if (errorHandler->hadError()) {
    return;
  }

  SemanticAnalyzer semantic(errorHandler, resolver, filename);
  const SemanticAnalyzeResult semanticResult =
      semantic.analyze(std::move(parseResult.declarations));
  if (errorHandler->hadError()) {
    return;
  }

  ScriptMetadata metadata;
  metadata.gameID = game::GameID::Skyrim;
  metadata.compilationTime = std::time(nullptr);
  metadata.sourceFile = fs::canonical(inputFile).string();
  metadata.userName = common::getUserName();
  metadata.computerName = common::getComputerName();

  Compiler compiler(errorHandler);
  pex::PexFile pexFile =
      compiler.compile(metadata, semanticResult.declarations);

  if (errorHandler->hadError()) {
    return;
  }

  auto outputFile = inputFile;
  outputFile.replace_extension(fs::path(".pex"));

  pexFile.writeToFile(outputFile.string());
}
}  // namespace vellum