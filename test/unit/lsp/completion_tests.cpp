#include <catch2/catch_test_macros.hpp>

#include "lsp_fixture.h"

using namespace vellum;
using vellum::lsp_test::LspTestFixture;

namespace {

constexpr const char* kMinimalScript = R"(script TrainingMannequin {
	var itemTypeBow = 7
	var skillMarksman = "Marksman"

	fun onHit() {
		var player = 0
		itemTypeBow
	}
}
)";

constexpr const char* kUndefinedAdvScript = R"(script TrainingMannequin {
	var itemTypeBow = 7
	fun advanceSkill(skill: String, gain: Float) {}
	fun onHit() {
		adv
	}
}
)";

constexpr const char* kUsesMathEx = R"(script TrainingMannequin {
  fun onHit() {
    Math
  }
}
)";

constexpr const char* kUsesMathDot = R"(script TrainingMannequin {
  fun onHit() {
    Math.
  }
}
)";

constexpr const char* kEventParamDot = R"(script TrainingMannequin {
  event onHit(aggressor: ObjectReference, source: Form, p: Projectile) {
    p.Get
  }
}
)";

constexpr const char* kGameDotInline = R"(script TrainingMannequin {
  event onHit() {
    Game.
  }
}
)";

constexpr const char* kSelfDot = R"(script TrainingMannequin {
  var itemTypeBow = 7
  fun onHit() {
    self.
  }
}
)";

constexpr const char* kSelfInlineDot = R"(script TrainingMannequin {
  var itemTypeBow = 7
  fun onHit() {
    self.GoToState
  }
}
)";

constexpr const char* kSuperDot = R"(script TrainingMannequin : ObjectReference {
  fun foo() {
    super.
  }
}
)";

constexpr const char* kSuperInlineDot = R"(script TrainingMannequin : ObjectReference {
  fun foo() {
    super.Enable
  }
}
)";

constexpr const char* kCastDot = R"(script TrainingMannequin {
  event onHit(source: Form) {
    source as Weapon.
  }
}
)";

constexpr const char* kCastParenDot = R"(script TrainingMannequin {
  event onHit(source: Form) {
    (source as Weapon).
  }
}
)";

constexpr const char* kCastInlineDot = R"(script TrainingMannequin {
  event onHit(source: Form) {
    source as Weapon.Fire
  }
}
)";

void addObjectReferenceModule(LspTestFixture& fixture) {
  VellumObject objectReference(VellumType::identifier("ObjectReference"));
  objectReference.addFunction(
      LspTestFixture::instanceFunction(VellumIdentifier("Enable")));
  fixture.addScriptType(objectReference);
}

void addFormHierarchy(LspTestFixture& fixture) {
  addObjectReferenceModule(fixture);

  VellumObject form(VellumType::identifier("Form"));
  form.addFunction(LspTestFixture::instanceFunction(VellumIdentifier("GetFormID")));
  fixture.addScriptType(form);

  VellumObject projectile(VellumType::identifier("Projectile"));
  fixture.addScriptType(projectile, VellumIdentifier("Form"));

  VellumObject package(VellumType::identifier("Package"));
  package.addFunction(
      LspTestFixture::instanceFunction(VellumIdentifier("GetOwningQuest")));
  fixture.addScriptType(package);
}

void addGameModule(LspTestFixture& fixture) {
  VellumObject game(VellumType::identifier("Game"));
  game.addFunction(LspTestFixture::staticFunction(VellumIdentifier("GetPlayer")));
  fixture.addScriptType(game);
}

void addWeaponModule(LspTestFixture& fixture) {
  VellumObject weapon(VellumType::identifier("Weapon"));
  weapon.addFunction(LspTestFixture::instanceFunction(VellumIdentifier("Fire")));
  fixture.addScriptType(weapon);
}

void addMathModule(LspTestFixture& fixture) {
  VellumObject math(VellumType::identifier("Math"));
  math.addFunction(LspTestFixture::staticFunction(VellumIdentifier("abs")));
  fixture.addScriptType(math);
}

void addMathExModule(LspTestFixture& fixture) {
  VellumObject mathEx(VellumType::identifier("MathEx"));
  mathEx.addFunction(LspTestFixture::staticFunction(VellumIdentifier("chance")));
  fixture.addScriptType(mathEx);
}

}  // namespace

TEST_CASE_METHOD(LspTestFixture, "LspCompletion expression and keyword contexts") {
  openDoc(kMinimalScript);
  const auto& cache = analyze();
  REQUIRE(cache.navigation);
  REQUIRE(cache.navigation->semanticOk);

  const auto exprList = complete(6, 9);
  REQUIRE(LspTestFixture::hasLabel(exprList, "itemTypeBow"));

  const auto kwList = complete(5, 2);
  REQUIRE(LspTestFixture::hasLabel(kwList, "var"));
}

TEST_CASE_METHOD(LspTestFixture,
                 "LspCompletion undefined identifier is best-effort") {
  openDoc(kUndefinedAdvScript);
  const auto& cache = analyze();
  REQUIRE(cache.navigation);
  REQUIRE(cache.navigation->semanticOk);
  REQUIRE_FALSE(cache.diagnostics.empty());

  REQUIRE(LspTestFixture::hasLabel(complete(4, 5), "advanceSkill"));
  REQUIRE(LspTestFixture::hasLabel(complete(4, 6), "advanceSkill"));
}

TEST_CASE_METHOD(LspTestFixture, "LspCompletion import module prefix") {
  addMathExModule(*this);
  openDoc(kUsesMathEx);
  analyze();

  REQUIRE(LspTestFixture::hasLabel(complete(2, 7), "MathEx"));
}

TEST_CASE_METHOD(LspTestFixture, "LspCompletion MathEx member when only MathEx exists") {
  addMathExModule(*this);
  openDoc(kUsesMathDot);
  analyze();

  const auto list = completeDot(2, 8);
  REQUIRE(LspTestFixture::hasLabel(list, "chance"));
}

TEST_CASE_METHOD(LspTestFixture, "LspCompletion Math module disambiguation") {
  addMathExModule(*this);
  addMathModule(*this);
  openDoc(kUsesMathDot);
  analyze();

  const auto list = completeDot(2, 8);
  REQUIRE(LspTestFixture::hasLabel(list, "abs"));
  REQUIRE_FALSE(LspTestFixture::hasLabel(list, "chance"));
}

TEST_CASE_METHOD(LspTestFixture, "LspCompletion Game static members") {
  addGameModule(*this);
  openDoc(kGameDotInline);
  analyze();

  REQUIRE(LspTestFixture::hasLabel(completeDot(2, 8), "GetPlayer"));
}

TEST_CASE_METHOD(LspTestFixture, "LspCompletion event parameter typed members") {
  addFormHierarchy(*this);
  openDoc(kEventParamDot);
  analyze();

  const auto list = completeDot(2, 6);
  REQUIRE(LspTestFixture::hasLabel(list, "GetFormID"));
  REQUIRE_FALSE(LspTestFixture::hasLabel(list, "GetOwningQuest"));
}

TEST_CASE_METHOD(LspTestFixture, "LspCompletion self member access") {
  openDoc(kSelfDot);
  analyze();
  REQUIRE(LspTestFixture::hasLabel(completeDot(3, 9), "itemTypeBow"));

  openDoc(kSelfInlineDot);
  analyze();
  REQUIRE(LspTestFixture::hasLabel(completeDot(3, 9), "itemTypeBow"));
}

TEST_CASE_METHOD(LspTestFixture, "LspCompletion super member access") {
  addObjectReferenceModule(*this);
  openDoc(kSuperDot);
  analyze();
  REQUIRE(LspTestFixture::hasLabel(completeDot(2, 10), "Enable"));

  openDoc(kSuperInlineDot);
  analyze();
  REQUIRE(LspTestFixture::hasLabel(completeDot(2, 10), "Enable"));
}

TEST_CASE_METHOD(LspTestFixture, "LspCompletion cast member access") {
  addWeaponModule(*this);
  addFormHierarchy(*this);

  openDoc(kCastDot);
  analyze();
  REQUIRE(LspTestFixture::hasLabel(completeDot(2, 21), "Fire"));

  openDoc(kCastParenDot);
  analyze();
  REQUIRE(LspTestFixture::hasLabel(completeDot(2, 23), "Fire"));

  openDoc(kCastInlineDot);
  analyze();
  REQUIRE(LspTestFixture::hasLabel(completeDot(2, 21), "Fire"));
}

TEST_CASE_METHOD(LspTestFixture, "LspCompletion bare import static call") {
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
    Rand
  }
}
)");
  analyze();

  REQUIRE(LspTestFixture::hasLabel(complete(5, 5), "RandomInt"));
}

TEST_CASE_METHOD(LspTestFixture, "LspCompletion ambiguous bare import statics") {
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
    shared
  }
}
)");
  analyze();

  const auto list = complete(6, 7);
  REQUIRE(LspTestFixture::hasLabelWithDetail(list, "sharedFunc",
                                             "ImportA.sharedFunc"));
  REQUIRE(LspTestFixture::hasLabelWithDetail(list, "sharedFunc",
                                             "ImportB.sharedFunc"));
}

TEST_CASE_METHOD(LspTestFixture, "LspCompletion bare import shadowed by local static") {
  const path utilityPath = path("/test/Utility.vel");
  addParsedVelModule(utilityPath, R"(script Utility {
  static fun sharedHelper() {}
}
)");
  openDoc(R"(import Utility

script TrainingMannequin {
  static fun sharedHelper() {}

  fun onHit() {
    shared
  }
}
)");
  analyze();

  const auto list = complete(7, 7);
  REQUIRE(LspTestFixture::hasLabel(list, "sharedHelper"));
  REQUIRE_FALSE(LspTestFixture::hasLabelWithDetail(list, "sharedHelper",
                                                   "Utility.sharedHelper"));
}
