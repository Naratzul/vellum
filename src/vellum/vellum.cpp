#include "vellum.h"

#include <ctime>

#include "analyze/semantic_analyzer.h"
#include "common/fs.h"
#include "common/os.h"
#include "compiler/compiler.h"
#include "compiler/compiler_error_handler.h"
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
  if (errorHandler->hadError()) {
    errorHandler->printErrors();
    return;
  }

  SemanticAnalyzer semantic(errorHandler);
  const SemanticAnalyzeResult semanticResult =
      semantic.analyze(std::move(parseResult.declarations));
  if (errorHandler->hadError()) {
    errorHandler->printErrors();
    return;
  }

  ScriptMetadata metadata;
  metadata.gameID = game::GameID::Skyrim;
  metadata.compilationTime = std::time(nullptr);
  metadata.sourceFile = common::canonicalPath(inputFile);
  metadata.userName = common::getUserName();
  metadata.computerName = common::getComputerName();

  Compiler compiler(errorHandler);
  pex::PexFile pexFile = compiler.compile(metadata, semanticResult.declarations);

  pexFile.writeToFile(common::replaceExtension(inputFile, ".pex"));
}
}  // namespace vellum