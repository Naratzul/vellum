#include "import_library.h"

#include <format>

#include "common/os.h"
#include "common/string_set.h"
#include "vellum/vellum_identifier.h"

namespace vellum {
using namespace common;

ImportLibrary::ImportLibrary(const Vec<fs::path>& importPaths) {
  scanImportPaths(importPaths);
}

void ImportLibrary::scanImportPaths(const Vec<fs::path>& importPaths) {
  for (const auto& path : importPaths) {
    if (!fs::is_directory(path)) {
      throw std::runtime_error("Import path is not a directory: " +
                               pathToUtf8(path));
    }

    for (const auto& entry : fs::recursive_directory_iterator(path)) {
      if (fs::is_regular_file(entry.path())) {
        auto filename = pathToUtf8(entry.path().stem());
        auto extension = pathToUtf8(entry.path().extension());

        if (extension != ".vel" && extension != ".psc") {
          continue;
        }

        VellumIdentifier name(StringSet::insert(filename));
        if (importNameToModule.contains(name)) {
          throw std::runtime_error(
              std::format("Duplicate import name detected: {} in {}",
                          name.toString(), pathToUtf8(entry.path())));
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
