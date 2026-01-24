#pragma once

#include <filesystem>

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

class ImportLibrary {
 public:
  ImportLibrary(const Vec<std::string>& importPaths);

  ImportModulePtr findModule(VellumIdentifier name) const;
  bool hasModule(VellumIdentifier name) const;

  // For testing: add a module manually
  void addTestModule(ImportModulePtr module);

 private:
  Map<VellumIdentifier, ImportModulePtr> importNameToModule;

  void scanImportPaths(const Vec<std::string>& importPaths);
};

}  // namespace vellum
