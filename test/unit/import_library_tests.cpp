#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <filesystem>
#include <fstream>

#include "analyze/import_library.h"
#include "common/fs.h"
#include "common/types.h"

namespace fs = std::filesystem;

using namespace vellum;
using common::Vec;

namespace {

void writeVelScript(const fs::path& path, std::string_view content) {
  fs::create_directories(path.parent_path());
  std::ofstream out(path);
  out << content;
}

}  // namespace

TEST_CASE("ImportLibrary_OverlappingImportPaths_SameFileSkipped") {
  const fs::path root =
      fs::temp_directory_path() / "vellum_import_overlap_test";
  const fs::path subDir = root / "scripts";
  const fs::path scriptPath = subDir / "MyScript.vel";
  fs::remove_all(root);

  writeVelScript(scriptPath, "script MyScript {}\n");

  ImportLibrary library(Vec<fs::path>{root, subDir});
  REQUIRE(library.hasModule(VellumIdentifier("MyScript")));
  REQUIRE(library.getScanWarnings().empty());

  fs::remove_all(root);
}

TEST_CASE("ImportLibrary_DuplicateStemDifferentFiles_StrictThrows") {
  const fs::path root =
      fs::temp_directory_path() / "vellum_import_conflict_test";
  const fs::path firstPath = root / "a" / "Conflict.vel";
  const fs::path secondPath = root / "b" / "Conflict.vel";
  fs::remove_all(root);

  writeVelScript(firstPath, "script Conflict {}\n");
  writeVelScript(secondPath, "script Conflict {}\n");

  REQUIRE_THROWS_WITH(
      ImportLibrary(Vec<fs::path>{root}, true),
      Catch::Matchers::ContainsSubstring("Duplicate import name detected"));

  fs::remove_all(root);
}

TEST_CASE("ImportLibrary_DuplicateStemDifferentFiles_LenientLastWins") {
  const fs::path root =
      fs::temp_directory_path() / "vellum_import_lenient_conflict_test";
  const fs::path firstPath = root / "a" / "Conflict.vel";
  const fs::path secondPath = root / "b" / "Conflict.vel";
  fs::remove_all(root);

  writeVelScript(firstPath, "script Conflict {}\n");
  writeVelScript(secondPath, "script Conflict {}\n");

  ImportLibrary library(Vec<fs::path>{root / "a", root / "b"});
  REQUIRE(library.hasModule(VellumIdentifier("Conflict")));
  REQUIRE(library.getScanWarnings().size() == 1);
  REQUIRE_THAT(library.getScanWarnings().front().message,
               Catch::Matchers::ContainsSubstring(
                   "Duplicate import name detected"));

  const auto module = library.findModule(VellumIdentifier("Conflict"));
  REQUIRE(module != nullptr);
  REQUIRE(common::canonicalPathKey(module->getFilePath()) ==
          common::canonicalPathKey(secondPath));

  fs::remove_all(root);
}

TEST_CASE("ImportLibrary_InvalidImportPath_LenientSkipsWithWarning") {
  const fs::path root =
      fs::temp_directory_path() / "vellum_import_invalid_path_test";
  const fs::path scriptPath = root / "Valid.vel";
  const fs::path missingPath = root / "missing";
  fs::remove_all(root);

  writeVelScript(scriptPath, "script Valid {}\n");

  ImportLibrary library(Vec<fs::path>{missingPath, root});
  REQUIRE(library.hasModule(VellumIdentifier("Valid")));
  REQUIRE(library.getScanWarnings().size() == 1);
  REQUIRE_THAT(library.getScanWarnings().front().message,
               Catch::Matchers::ContainsSubstring(
                   "Import path is not a directory"));

  fs::remove_all(root);
}

TEST_CASE("ImportLibrary_InvalidImportPath_StrictThrows") {
  const fs::path root =
      fs::temp_directory_path() / "vellum_import_invalid_path_strict_test";
  const fs::path missingPath = root / "missing";
  fs::remove_all(root);

  REQUIRE_THROWS_WITH(
      ImportLibrary(Vec<fs::path>{missingPath}, true),
      Catch::Matchers::ContainsSubstring("Import path is not a directory"));

  fs::remove_all(root);
}
