#include "import_resolver.h"

#include <algorithm>
#include <format>

#include "analyze/declaration_analysis.h"
#include "analyze/type_collector.h"
#include "common/fs.h"
#include "common/os.h"
#include "compiler/builtin_functions.h"
#include "compiler/resolver.h"
#include "lexer/lexer.h"
#include "parser/papyrus_lexer.h"
#include "parser/papyrus_parser.h"
#include "vellum/vellum_identifier.h"
#include "vellum/vellum_literal.h"

namespace vellum {
using namespace common;

ImportResolver::ImportResolver(const Shared<CompilerErrorHandler>& errorHandler,
                               const Shared<ImportLibrary>& importLibrary)
    : errorHandler(errorHandler), importLibrary(importLibrary) {}

void ImportResolver::buildImportGraph(
    const Set<VellumIdentifier>& importedNames,
    Opt<VellumIdentifier> compilingScript) {
  discoveredTypes.clear();
  this->compilingScript = std::move(compilingScript);
  doBuildImportGraph(importedNames);
  doResolveAllModules();
  this->compilingScript = std::nullopt;
}

void ImportResolver::ensureModule(VellumIdentifier name) {
  const auto module = importLibrary->findModule(name);
  if (!module) {
    return;
  }
  if (!module->isParsed()) {
    doBuildImportGraph(Set<VellumIdentifier>{name});
  }
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
    if (compilingScript &&
        equalsCaseInsensitive(name, *compilingScript)) {
      continue;
    }

    auto import = importLibrary->findModule(name);

    auto [_, added] = discoveredTypes.insert(name);
    if (!added) {
      continue;
    }

    if (!import) {
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

    const auto filename = pathToUtf8(module->getFilePath().stem());

    auto builtinFunctions = makeShared<BuiltinFunctions>();
    auto resolver =
        makeShared<Resolver>(VellumObject(VellumType::identifier(name)),
                             errorHandler, importLibrary, builtinFunctions);

    module->setResolver(resolver);

    collectDeclarations(module->getAst().declarations, errorHandler, resolver,
                      filename);
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