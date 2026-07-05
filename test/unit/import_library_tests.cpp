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

  fs::remove_all(root);
}

TEST_CASE("ImportLibrary_DuplicateStemDifferentFiles_Throws") {
  const fs::path root =
      fs::temp_directory_path() / "vellum_import_conflict_test";
  const fs::path firstPath = root / "a" / "Conflict.vel";
  const fs::path secondPath = root / "b" / "Conflict.vel";
  fs::remove_all(root);

  writeVelScript(firstPath, "script Conflict {}\n");
  writeVelScript(secondPath, "script Conflict {}\n");

  REQUIRE_THROWS_WITH(
      ImportLibrary(Vec<fs::path>{root}),
      Catch::Matchers::ContainsSubstring("Duplicate import name detected"));

  fs::remove_all(root);
}
