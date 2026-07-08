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

constexpr const char* kSuperProperty =
    R"(script TrainingMannequin : ObjectReference {
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
  const path mathExPath = path("/test/MathEx.vel");
  addParsedVelModule(mathExPath, R"(script MathEx {
  fun onHit() {}
}
)");
  openDoc(kImportMathEx);
  analyze();
  const auto links = define(0, 7);
  REQUIRE_FALSE(links.empty());
  REQUIRE(LspTestFixture::definesInFile(links, mathExPath));
  REQUIRE(LspTestFixture::definesOnLine(links, 0));
}

TEST_CASE_METHOD(LspTestFixture, "LspDefinition returns empty on whitespace") {
  openDoc(kEmptyBody);
  analyze();

  REQUIRE(define(2, 2).empty());
}

TEST_CASE_METHOD(LspTestFixture, "LspDefinition bare import static callee") {
  const path utilityPath = path("/test/Utility.vel");
  addParsedVelModule(utilityPath, R"(script Utility {
  static fun RandomInt(min: Int, max: Int) -> Int {
    return 0
  }
}
)");
  openDoc(R"(import Utility

script TrainingMannequin {
  fun onHit() {
    RandomInt(0, 10)
  }
}
)");
  analyze();

  const auto links = define(4, 4);
  REQUIRE_FALSE(links.empty());
  REQUIRE(LspTestFixture::definesInFile(links, utilityPath));
  REQUIRE(LspTestFixture::definesOnLine(links, 1));
}

TEST_CASE_METHOD(LspTestFixture, "LspDefinition ambiguous bare import callee") {
  const path importAPath = path("/test/ImportA.vel");
  const path importBPath = path("/test/ImportB.vel");
  addParsedVelModule(importAPath, R"(script ImportA {
  static fun sharedFunc() {}
}
)");
  addParsedVelModule(importBPath, R"(script ImportB {
  static fun sharedFunc() {}
}
)");
  openDoc(R"(import ImportA
import ImportB

script TrainingMannequin {
  fun onHit() {
    sharedFunc()
  }
}
)");
  analyze();

  REQUIRE(define(5, 4).empty());
}
