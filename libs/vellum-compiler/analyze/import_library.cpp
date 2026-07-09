#include "import_library.h"

#include <format>

#include "common/fs.h"
#include "common/os.h"
#include "common/string_set.h"
#include "vellum/vellum_identifier.h"

namespace vellum {
using namespace common;

ImportLibrary::ImportLibrary(const Vec<fs::path>& importPaths,
                               bool strictDuplicates)
    : strictDuplicates(strictDuplicates) {
  scanImportPaths(importPaths);
}

void ImportLibrary::scanImportPaths(const Vec<fs::path>& importPaths) {
  for (const auto& path : importPaths) {
    if (!fs::is_directory(path)) {
      const auto message =
          std::format("Import path is not a directory: {}", pathToUtf8(path));
      if (strictDuplicates) {
        throw std::runtime_error(message);
      }
      scanWarnings.push_back({message});
      continue;
    }

    for (const auto& entry : fs::recursive_directory_iterator(path)) {
      if (!fs::is_regular_file(entry.path())) {
        continue;
      }

      auto filename = pathToUtf8(entry.path().stem());
      auto extension = pathToUtf8(entry.path().extension());

      if (extension != ".vel" && extension != ".psc") {
        continue;
      }

      const auto moduleType = extension == ".vel" ? ImportModuleType::Vellum
                                                  : ImportModuleType::Papyrus;
      VellumIdentifier name(StringSet::insert(filename));
      if (auto it = importNameToModule.find(name);
          it != importNameToModule.end()) {
        if (canonicalPathKey(it->second->getFilePath()) ==
            canonicalPathKey(entry.path())) {
          continue;
        }

        const auto message = std::format(
            "Duplicate import name detected: {} in {} (overriding {})",
            name.toString(), pathToUtf8(entry.path()),
            pathToUtf8(it->second->getFilePath()));
        if (strictDuplicates) {
          throw std::runtime_error(message);
        }
        scanWarnings.push_back({message});
      }

      registerModule(name, moduleType, entry.path());
    }
  }
}

void ImportLibrary::registerModule(VellumIdentifier name,
                                   ImportModuleType type,
                                   const fs::path& filePath) {
  importNameToModule[name] = makeShared<ImportModule>(name, type, filePath);
}

ImportModulePtr ImportLibrary::findModule(VellumIdentifier name) const {
  // First try case-sensitive lookup (for Vellum modules)
  auto it = importNameToModule.find(name);
  if (it != importNameToModule.end()) {
    return it->second;
  }

  // For case-insensitive lookup (Papyrus modules), iterate through all modules
  for (const auto& [moduleName, module] : importNameToModule) {
    if (module->getType() == ImportModuleType::Papyrus) {
      if (equalsCaseInsensitive(moduleName, name)) {
        return module;
      }
    }
  }

  return nullptr;
}

bool ImportLibrary::hasModule(VellumIdentifier name) const {
  return importNameToModule.contains(name);
}

void ImportLibrary::addTestModule(ImportModulePtr module) {
  importNameToModule[module->getName()] = module;
}

}  // namespace vellum
