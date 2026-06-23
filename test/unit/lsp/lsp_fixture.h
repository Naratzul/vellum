#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "analyze/import_library.h"
#include "common/fs.h"
#include "common/types.h"
#include "compiler/builtin_functions.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "completions_provider.h"
#include "document_store.h"
#include "lsp_locations.h"
#include "vellum/vellum_type.h"
#include "vellum/vellum_function.h"
#include "vellum/vellum_object.h"

namespace vellum::lsp_test {

using common::makeShared;
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

  static bool hasLabel(const lsp::CompletionList& list, std::string_view label) {
    for (const auto& item : list.items) {
      if (item.label == label) {
        return true;
      }
    }
    return false;
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

  static VellumFunction staticFunction(VellumIdentifier name) {
    return VellumFunction(name, VellumType::none(), {}, true);
  }

  static VellumFunction instanceFunction(VellumIdentifier name) {
    return VellumFunction(name, VellumType::none(), {}, false);
  }

 protected:
  DocumentStore store;
  Shared<ImportLibrary> importLibrary;
  path filePath;
  Shared<CompilerErrorHandler> errorHandler = makeShared<CompilerErrorHandler>();
};

}  // namespace vellum::lsp_test
