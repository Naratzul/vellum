#pragma once

#include <filesystem>
#include <string>

#include "common/types.h"
#include "parser/parser.h"
#include "vellum/vellum_identifier.h"

namespace fs = std::filesystem;

namespace vellum {
using common::Map;
using common::Shared;
using common::Vec;

class Resolver;

enum class ImportModuleType {
  Vellum,
  Papyrus,
};

class ImportModule {
 public:
  ImportModule(VellumIdentifier name, ImportModuleType type,
               const fs::path& filePath)
      : name(name), type(type), filePath(filePath) {}

  const VellumIdentifier& getName() const { return name; }

  ImportModuleType getType() const { return type; }

  const fs::path& getFilePath() const { return filePath; }

  const Shared<Resolver>& getResolver() const { return resolver; }
  void setResolver(const Shared<Resolver>& res) { resolver = res; }

  const ParserResult& getAst() const { return ast; }
  ParserResult& getAst() { return ast; }
  void setAst(ParserResult&& pr) { ast = std::move(pr); }

  std::string_view getFileContent() const { return fileContent; }
  void setFileContent(const std::string& str) { fileContent = str; }

  bool isParsed() const { return !fileContent.empty(); }

 private:
  VellumIdentifier name;
  ImportModuleType type;
  fs::path filePath;
  std::string fileContent;
  ParserResult ast;
  Shared<Resolver> resolver;
  Vec<Shared<ImportModule>> dependencies;
};

using ImportModulePtr = Shared<ImportModule>;

struct ImportScanWarning {
  std::string message;
};

class ImportLibrary {
 public:
  explicit ImportLibrary(const Vec<fs::path>& importPaths,
                         bool strictDuplicates = false);

  ImportModulePtr findModule(VellumIdentifier name) const;
  bool hasModule(VellumIdentifier name) const;

  const Map<VellumIdentifier, ImportModulePtr>& getAllModules() const {
    return importNameToModule;
  }

  const Vec<ImportScanWarning>& getScanWarnings() const { return scanWarnings; }

  void addTestModule(ImportModulePtr module);

 private:
  Map<VellumIdentifier, ImportModulePtr> importNameToModule;
  Vec<ImportScanWarning> scanWarnings;
  bool strictDuplicates;

  void scanImportPaths(const Vec<fs::path>& importPaths);
  void registerModule(VellumIdentifier name, ImportModuleType type,
                      const fs::path& filePath);
};

}  // namespace vellum
