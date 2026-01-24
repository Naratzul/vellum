#include "import_library.h"
#include "common/string_set.h"

#include <format>

namespace vellum {
using namespace common;

ImportLibrary::ImportLibrary(const Vec<std::string>& importPaths) {
  scanImportPaths(importPaths);
}

void ImportLibrary::scanImportPaths(const Vec<std::string>& importPaths) {
  for (const auto& importPath : importPaths) {
    fs::path path(importPath);
    if (!fs::is_directory(path)) {
      throw std::runtime_error("Import path is not a directory: " + importPath);
    }

    for (const auto& entry : fs::recursive_directory_iterator(path)) {
      if (fs::is_regular_file(entry.path())) {
        auto filename = entry.path().stem().string();
        auto extension = entry.path().extension().string();

        VellumIdentifier name(StringSet::insert(filename));
        if (importNameToModule.contains(name)) {
          throw std::runtime_error(
              std::format("Duplicate import name detected: {} in {}",
                          name.toString(), entry.path().string()));
        }

        if (extension == ".vel") {
          importNameToModule[name] = makeShared<ImportModule>(
              name, ImportModuleType::Vellum, entry.path());
        } else if (extension == ".psc") {
          importNameToModule[name] = makeShared<ImportModule>(
              name, ImportModuleType::Papyrus, entry.path());
        }
      }
    }
  }
}

ImportModulePtr ImportLibrary::findModule(VellumIdentifier name) const {
  auto it = importNameToModule.find(name);
  if (it == importNameToModule.end()) {
    return nullptr;
  }
  return it->second;
}

bool ImportLibrary::hasModule(VellumIdentifier name) const {
  return importNameToModule.contains(name);
}

void ImportLibrary::addTestModule(ImportModulePtr module) {
  importNameToModule[module->getName()] = module;
}

}  // namespace vellum
