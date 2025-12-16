#include "vellum.h"

#include <ctime>

#include "analyze/semantic_analyzer.h"
#include "common/fs.h"
#include "common/os.h"
#include "common/string_set.h"
#include "compiler/compiler.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "pex/pex_file.h"

namespace vellum {
void Vellum::run(std::string_view inputFile) {
  const std::string& sourceCode = common::readFileContent(inputFile);
  auto lexer = std::make_unique<Lexer>(sourceCode);
  auto errorHandler = std::make_shared<CompilerErrorHandler>();

  Parser parser(std::move(lexer), errorHandler);
  ParserResult parseResult = parser.parse();

  VellumObject debug(VellumType::identifier("Debug"));
  debug.addFunction(VellumFunction(
      VellumIdentifier("messageBox"), VellumType::none(),
      {VellumVariable(VellumIdentifier("message"),
                      VellumType::literal(VellumLiteralType::String))}));
  debug.addFunction(VellumFunction(
      VellumIdentifier("notification"), VellumType::none(),
      {VellumVariable(VellumIdentifier("message"),
                      VellumType::literal(VellumLiteralType::String))}));

  parseResult.resolver->importObject(debug);

  SemanticAnalyzer semantic(errorHandler, parseResult.resolver, common::filenameWithoutExt(inputFile));
  const SemanticAnalyzeResult semanticResult =
      semantic.analyze(std::move(parseResult.declarations));
  if (errorHandler->hadError()) {
    return;
  }

  ScriptMetadata metadata;
  metadata.gameID = game::GameID::Skyrim;
  metadata.compilationTime = std::time(nullptr);
  metadata.sourceFile = common::canonicalPath(inputFile);
  metadata.userName = common::getUserName();
  metadata.computerName = common::getComputerName();

  Compiler compiler(errorHandler);
  pex::PexFile pexFile =
      compiler.compile(metadata, semanticResult.declarations);

  if (errorHandler->hadError()) {
    return;
  }

  pexFile.writeToFile(common::replaceExtension(inputFile, ".pex"));
}
}  // namespace vellum