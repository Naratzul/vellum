#include <catch2/catch_test_macros.hpp>

#include <filesystem>

#include "lsp_fixture.h"

namespace fs = std::filesystem;

using namespace vellum;
using common::Vec;
using vellum::lsp_test::LspTestFixture;

namespace {

constexpr const char* kGlobalVarUse = R"(script TrainingMannequin {
  var itemTypeBow = 7
  fun onHit() {
    itemTypeBow
  }
}
)";

constexpr const char* kLocalVarUse = R"(script TrainingMannequin {
  fun onHit() {
    var player = 0
    player
  }
}
)";

constexpr const char* kFunctionCall = R"(script TrainingMannequin {
  fun advanceSkill(skill: String, gain: Float) {}
  fun onHit() {
    advanceSkill("Marksman", 1.0)
  }
}
)";

constexpr const char* kSelfProperty = R"(script TrainingMannequin {
  var itemTypeBow = 7
  fun onHit() {
    self.itemTypeBow
  }
}
)";

constexpr const char* kSuperProperty = R"(script TrainingMannequin : ObjectReference {
  fun onHit() {
    super.Enable
  }
}
)";

constexpr const char* kImportMathEx = R"(import MathEx

script TrainingMannequin {
  fun onHit() {}
}
)";

constexpr const char* kObjectReferenceModule = R"(script ObjectReference {
  fun Enable() {}
}
)";

constexpr const char* kEmptyBody = R"(script TrainingMannequin {
  fun onHit() {
  }
}
)";

}  // namespace

TEST_CASE_METHOD(LspTestFixture, "LspDefinition global variable use") {
  openDoc(kGlobalVarUse);
  analyze();

  const auto links = define(3, 4);
  REQUIRE_FALSE(links.empty());
  REQUIRE(LspTestFixture::definesInFile(links, docPath()));
  REQUIRE(LspTestFixture::definesOnLine(links, 1));
}

TEST_CASE_METHOD(LspTestFixture, "LspDefinition local variable use") {
  openDoc(kLocalVarUse);
  analyze();

  const auto links = define(3, 4);
  REQUIRE_FALSE(links.empty());
  REQUIRE(LspTestFixture::definesInFile(links, docPath()));
  REQUIRE(LspTestFixture::definesOnLine(links, 2));
}

TEST_CASE_METHOD(LspTestFixture, "LspDefinition function call callee") {
  openDoc(kFunctionCall);
  analyze();

  const auto links = define(3, 4);
  REQUIRE_FALSE(links.empty());
  REQUIRE(LspTestFixture::definesInFile(links, docPath()));
  REQUIRE(LspTestFixture::definesOnLine(links, 1));
}

TEST_CASE_METHOD(LspTestFixture, "LspDefinition function declaration name") {
  openDoc(kFunctionCall);
  analyze();

  const auto links = define(1, 6);
  REQUIRE_FALSE(links.empty());
  REQUIRE(LspTestFixture::definesInFile(links, docPath()));
  REQUIRE(LspTestFixture::definesOnLine(links, 1));
}

TEST_CASE_METHOD(LspTestFixture, "LspDefinition self property") {
  openDoc(kSelfProperty);
  analyze();

  const auto links = define(3, 9);
  REQUIRE_FALSE(links.empty());
  REQUIRE(LspTestFixture::definesInFile(links, docPath()));
  REQUIRE(LspTestFixture::definesOnLine(links, 1));
}

TEST_CASE_METHOD(LspTestFixture, "LspDefinition super property") {
  const path objectReferencePath = path("/test/ObjectReference.vel");
  addParsedVelModule(objectReferencePath, kObjectReferenceModule);
  openDoc(kSuperProperty);
  analyze();

  const auto links = define(2, 11);
  REQUIRE_FALSE(links.empty());
  REQUIRE(LspTestFixture::definesInFile(links, objectReferencePath));
  REQUIRE(LspTestFixture::definesOnLine(links, 1));
}

TEST_CASE_METHOD(LspTestFixture, "LspDefinition import name") {
  setImportPaths(Vec<fs::path>{fs::path(VELLUM_SOURCE_DIR) / "examples"});
  openDoc(kImportMathEx);
  analyze();

  const auto links = define(0, 7);
  const path mathExPath = fs::path(VELLUM_SOURCE_DIR) / "examples" / "MathEx.vel";
  REQUIRE_FALSE(links.empty());
  REQUIRE(LspTestFixture::definesInFile(links, mathExPath));
  REQUIRE(LspTestFixture::definesOnLine(links, 2));
}

TEST_CASE_METHOD(LspTestFixture, "LspDefinition returns empty on whitespace") {
  openDoc(kEmptyBody);
  analyze();

  REQUIRE(define(2, 2).empty());
}
