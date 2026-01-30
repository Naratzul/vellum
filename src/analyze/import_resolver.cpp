#include "import_resolver.h"

#include <algorithm>
#include <format>

#include "analyze/declaration_collector.h"
#include "analyze/type_collector.h"
#include "common/fs.h"
#include "compiler/resolver.h"
#include "lexer/lexer.h"
#include "parser/papyrus_lexer.h"
#include "parser/papyrus_parser.h"
#include "vellum/vellum_literal.h"

namespace vellum {
using namespace common;

ImportResolver::ImportResolver(const Shared<CompilerErrorHandler>& errorHandler,
                               const Shared<ImportLibrary>& importLibrary)
    : errorHandler(errorHandler), importLibrary(importLibrary) {}

void ImportResolver::buildImportGraph(
    const Set<VellumIdentifier>& importedNames) {
  for (const auto& name : importedNames) {
    auto import = importLibrary->findModule(name);

    if (!import) {
      // TODO: pass token
      errorHandler->errorAt(
          Token(), CompilerErrorKind::UndefinedIdentifier,
          "Undefined import name {}. Did you miss import path?",
          name.toString());
      continue;
    }

    // Skip if already resolved (handles cyclic imports)
    if (import->getResolver()) {
      continue;
    }

    parseImport(*import);

    const auto filename = import->getFilePath().stem().string();

    TypeCollector typeCollector;
    typeCollector.collect(import->getAst().declarations);

    buildImportGraph(typeCollector.getDiscoveredTypes());
    auto resolver =
        makeShared<Resolver>(VellumObject(VellumType::identifier(name)),
                             errorHandler, importLibrary);

    import->setResolver(resolver);

    bool isPapyrus = import->getType() == ImportModuleType::Papyrus;
    DeclarationCollector collector(errorHandler, resolver, filename, isPapyrus);
    collector.collect(import->getAst().declarations);
  }
}

const Shared<Resolver> ImportResolver::getImportResolver(
    VellumIdentifier name) {
  auto import = importLibrary->findModule(name);
  if (!import) {
    return nullptr;
  }
  return import->getResolver();
}

void ImportResolver::parseImport(ImportModule& import) {
  import.setFileContent(readFileContent(import.getFilePath()));

  switch (import.getType()) {
    case ImportModuleType::Vellum: {
      Parser parser(makeUnique<Lexer>(import.getFileContent()), errorHandler);
      import.setAst(parser.parse());
    } break;
    case ImportModuleType::Papyrus: {
      PapyrusParser parser(makeUnique<PapyrusLexer>(import.getFileContent()),
                           errorHandler);
      import.setAst(parser.parse());
    } break;
  }
}
}  // namespace vellum