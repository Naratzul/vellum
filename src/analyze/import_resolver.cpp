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
  discoveredTypes.clear();
  doBuildImportGraph(importedNames);
  doResolveAllModules();
}

const Shared<Resolver> ImportResolver::getImportResolver(
    VellumIdentifier name) {
  auto import = importLibrary->findModule(name);
  if (!import) {
    return nullptr;
  }
  return import->getResolver();
}

void ImportResolver::doBuildImportGraph(
    const Set<VellumIdentifier>& importedNames) {
  for (const auto& name : importedNames) {
    auto import = importLibrary->findModule(name);

    auto [_, added] = discoveredTypes.insert(name);
    if (!added) {
      continue;
    }

    if (!import) {
      // TODO: pass token
      errorHandler->errorAt(
          Token(), CompilerErrorKind::UndefinedIdentifier,
          "Undefined import name {}. Did you miss import path?",
          name.toString());
      continue;
    }

    // Skip if already parsed (handles cyclic imports)
    if (import->isParsed()) {
      continue;
    }

    parseImport(*import);

    TypeCollector typeCollector;
    typeCollector.collect(import->getAst().declarations);

    doBuildImportGraph(typeCollector.getDiscoveredTypes());
  }
}

void ImportResolver::doResolveAllModules() {
  for (const auto& [name, module] : importLibrary->getAllModules()) {
    // Skip if already resolved
    if (module->getResolver()) {
      continue;
    }

    // Skip if not parsed (module wasn't discovered)
    if (!module->isParsed()) {
      continue;
    }

    const auto filename = module->getFilePath().stem().string();

    auto resolver =
        makeShared<Resolver>(VellumObject(VellumType::identifier(name)),
                             errorHandler, importLibrary);

    module->setResolver(resolver);

    DeclarationCollector collector(errorHandler, resolver, filename);
    collector.collect(module->getAst().declarations);
  }
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