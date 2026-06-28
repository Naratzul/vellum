#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "analyze/import_library.h"
#include "analyze/declaration_collector.h"
#include "ast/decl/declaration.h"
#include "common/fs.h"
#include "common/types.h"
#include "compiler/builtin_functions.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "completions_provider.h"
#include "definitions_provider.h"
#include "document_store.h"
#include "lexer/lexer.h"
#include "lsp_locations.h"
#include "parser/parser.h"
#include "vellum/vellum_function.h"
#include "vellum/vellum_object.h"
#include "vellum/vellum_type.h"

namespace vellum::lsp_test {

using common::makeShared;
using common::makeUnique;
using common::Shared;
using common::Vec;
using common::fs::path;

class LspTestFixture {
 public:
  LspTestFixture() {
    importLibrary = makeShared<ImportLibrary>(Vec<path>{});
    filePath = path("/tmp/TrainingMannequin.vel");
  }

  void setImportPaths(const Vec<path>& paths) {
    importLibrary = makeShared<ImportLibrary>(paths);
  }

  void openDoc(std::string_view source) { store.openOrUpdate(filePath, std::string(source)); }

  const CachedAnalysis& analyze() {
    return store.getOrAnalyze(filePath, importLibrary);
  }

  lsp::CompletionList complete(unsigned line, unsigned character,
                               std::optional<lsp::CompletionContext> context = std::nullopt) {
    lsp::requests::TextDocument_Completion::Params params;
    params.textDocument.uri = pathToUri(filePath);
    params.position = lsp::Position{.line = line, .character = character};
    if (context) {
      params.context = *context;
    }
    const auto result =
        CompletionsProvider().getCompletions(filePath, params, store, importLibrary);
    if (const auto* list = std::get_if<lsp::CompletionList>(&result)) {
      return *list;
    }
    return {};
  }

  lsp::CompletionList completeDot(unsigned line, unsigned character) {
    return complete(line, character,
                    lsp::CompletionContext{
                        .triggerKind = lsp::CompletionTriggerKind::TriggerCharacter,
                        .triggerCharacter = "."});
  }

  lsp::Array<lsp::DefinitionLink> define(unsigned line, unsigned character) {
    return DefinitionsProvider().getDefinitions(
        filePath, lsp::Position{.line = line, .character = character}, store,
        importLibrary);
  }

  const path& docPath() const { return filePath; }

  static bool hasLabel(const lsp::CompletionList& list, std::string_view label) {
    for (const auto& item : list.items) {
      if (item.label == label) {
        return true;
      }
    }
    return false;
  }

  static bool definesOnLine(const lsp::Array<lsp::DefinitionLink>& links,
                            unsigned line) {
    return !links.empty() && links[0].targetRange.start.line == line;
  }

  static bool definesInFile(const lsp::Array<lsp::DefinitionLink>& links,
                            const path& expectedPath) {
    if (links.empty()) {
      return false;
    }
    const path targetPath{links[0].targetUri.path()};
    return targetPath == expectedPath ||
           targetPath.filename() == expectedPath.filename();
  }

  void addScriptType(const VellumObject& object,
                     std::optional<VellumIdentifier> parentName = std::nullopt) {
    const VellumIdentifier name = object.getType().asIdentifier();
    auto module =
        makeShared<ImportModule>(name, ImportModuleType::Papyrus, path(""));
    auto builtinFunctions = makeShared<BuiltinFunctions>();
    auto objectResolver =
        makeShared<Resolver>(object, errorHandler, importLibrary, builtinFunctions);
    if (parentName) {
      objectResolver->setParentType(VellumType::identifier(*parentName));
    }
    module->setResolver(objectResolver);
    module->setFileContent("// test module");
    importLibrary->addTestModule(module);
  }

  void addParsedVelModule(const path& modulePath, std::string_view source,
                          std::optional<VellumIdentifier> parentName =
                              std::nullopt) {
    auto parseErrors = makeShared<CompilerErrorHandler>();
    Parser parser(makeUnique<Lexer>(source), parseErrors);
    ParserResult parseResult = parser.parse();

    VellumIdentifier scriptName("");
    for (const auto& decl : parseResult.declarations) {
      if (auto* script = dynamic_cast<ast::ScriptDeclaration*>(decl.get())) {
        scriptName = script->getScriptName().asIdentifier();
        break;
      }
    }

    auto builtinFunctions = makeShared<BuiltinFunctions>();
    auto moduleResolver = makeShared<Resolver>(
        VellumObject(VellumType::identifier(scriptName)), errorHandler,
        importLibrary, builtinFunctions);
    if (parentName) {
      moduleResolver->setParentType(VellumType::identifier(*parentName));
    }

    DeclarationCollector collector(errorHandler, moduleResolver,
                                   scriptName.toString());
    collector.collect(parseResult.declarations);

    auto module =
        makeShared<ImportModule>(scriptName, ImportModuleType::Vellum, modulePath);
    module->setFileContent(std::string(source));
    module->setAst(std::move(parseResult));
    module->setResolver(moduleResolver);
    importLibrary->addTestModule(module);
  }

  static VellumFunction staticFunction(VellumIdentifier name) {
    return VellumFunction(name, VellumType::none(), {},
                          VellumFunctionModifier{VellumFunctionModifierBits::Static});
  }

  static VellumFunction instanceFunction(VellumIdentifier name) {
    return VellumFunction(name, VellumType::none(), {}, VellumFunctionModifier{});
  }

 protected:
  DocumentStore store;
  Shared<ImportLibrary> importLibrary;
  path filePath;
  Shared<CompilerErrorHandler> errorHandler = makeShared<CompilerErrorHandler>();
};

}  // namespace vellum::lsp_test
