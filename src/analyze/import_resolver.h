#pragma once

#include "analyze/import_library.h"
#include "common/types.h"
#include "vellum/vellum_object.h"

namespace vellum {
using common::Opt;
using common::Set;
using common::Shared;
using common::Vec;

class ImportResolver {
 public:
  ImportResolver(const Shared<CompilerErrorHandler>& errorHandler,
                 const Shared<ImportLibrary>& importLibrary);

  void buildImportGraph(const Set<VellumIdentifier>& importedNames);

  const Shared<Resolver> getImportResolver(VellumIdentifier name);

 private:
  Shared<CompilerErrorHandler> errorHandler;
  Shared<ImportLibrary> importLibrary;
  Set<VellumIdentifier> discoveredTypes;

  void doBuildImportGraph(const Set<VellumIdentifier>& importedNames);
  void doResolveAllModules();

  void parseImport(ImportModule& import);
};
}  // namespace vellum