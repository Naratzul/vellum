#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <string>

#include "analyze/import_library.h"
#include "document_store.h"

using namespace vellum;
using common::makeShared;
using common::Shared;
using common::Vec;
using common::fs::path;

namespace {

constexpr const char* kValidScript = R"(script TrainingMannequin {
  var itemTypeBow = 7
  fun onHit() {
    itemTypeBow
  }
}
)";

constexpr const char* kInvalidScript = R"(script TrainingMannequin {
  var itemTypeBow = 7
  fun onHit() {
    itemTypeBow
  }
)";

lsp::Range range(unsigned startLine, unsigned startChar, unsigned endLine,
                 unsigned endChar) {
  return lsp::Range{
      .start = lsp::Position{.line = startLine, .character = startChar},
      .end = lsp::Position{.line = endLine, .character = endChar},
  };
}

}  // namespace

TEST_CASE("DocumentStore openOrUpdate and has") {
  DocumentStore store;
  const path filePath("/tmp/TrainingMannequin.vel");

  REQUIRE_FALSE(store.has(filePath));
  store.openOrUpdate(filePath, kValidScript);
  REQUIRE(store.has(filePath));
  REQUIRE(store.text(filePath) == kValidScript);
  REQUIRE(store.scriptName(filePath) == "TrainingMannequin");
}

TEST_CASE("DocumentStore applyChange replaces text in range") {
  DocumentStore store;
  const path filePath("/tmp/TrainingMannequin.vel");
  store.openOrUpdate(filePath, "hello world");

  store.applyChange(filePath, range(0, 6, 0, 11), "vellum");
  REQUIRE(store.text(filePath) == "hello vellum");
}

TEST_CASE("DocumentStore applyChange inserts text at cursor") {
  DocumentStore store;
  const path filePath("/tmp/TrainingMannequin.vel");
  store.openOrUpdate(filePath, "abc");

  store.applyChange(filePath, range(0, 1, 0, 1), "XY");
  REQUIRE(store.text(filePath) == "aXYbc");
}

TEST_CASE("DocumentStore applyChange deletes text in range") {
  DocumentStore store;
  const path filePath("/tmp/TrainingMannequin.vel");
  store.openOrUpdate(filePath, "hello world");

  store.applyChange(filePath, range(0, 5, 0, 6), "");
  REQUIRE(store.text(filePath) == "helloworld");
}

TEST_CASE("DocumentStore applyChange handles multi-line range") {
  DocumentStore store;
  const path filePath("/tmp/TrainingMannequin.vel");
  store.openOrUpdate(filePath, "line one\nline two\nline three");

  store.applyChange(filePath, range(0, 5, 1, 5), "X");
  REQUIRE(store.text(filePath) == "line Xtwo\nline three");
}

TEST_CASE("DocumentStore applyChange uses UTF-16 character offsets") {
  DocumentStore store;
  const path filePath("/tmp/TrainingMannequin.vel");
  // UTF-8 for U+1F600 (grinning face) — one codepoint, two UTF-16 code units.
  const std::string text = std::string("a") + "\xF0\x9F\x98\x80" + "b";
  store.openOrUpdate(filePath, text);

  store.applyChange(filePath, range(0, 1, 0, 3), "X");
  REQUIRE(store.text(filePath) == std::string("a") + "X" + "b");
}

TEST_CASE("DocumentStore applyChange throws for unknown document") {
  DocumentStore store;
  const path filePath("/tmp/unknown.vel");

  REQUIRE_THROWS_AS(
      store.applyChange(filePath, range(0, 0, 0, 0), "x"),
      std::out_of_range);
}

TEST_CASE("DocumentStore applyChange invalidates analysis cache") {
  DocumentStore store;
  const path filePath("/tmp/TrainingMannequin.vel");
  Shared<ImportLibrary> importLibrary = makeShared<ImportLibrary>(Vec<path>{});

  store.openOrUpdate(filePath, kInvalidScript);
  const auto& before = store.getOrAnalyze(filePath, importLibrary);
  REQUIRE_FALSE(before.diagnostics.empty());

  store.applyChange(filePath, range(5, 0, 5, 0), "}\n");
  const auto& after = store.getOrAnalyze(filePath, importLibrary);
  REQUIRE(after.diagnostics.empty());
}

TEST_CASE("DocumentStore close removes document") {
  DocumentStore store;
  const path filePath("/tmp/TrainingMannequin.vel");

  store.openOrUpdate(filePath, kValidScript);
  REQUIRE(store.has(filePath));

  store.close(filePath);
  REQUIRE_FALSE(store.has(filePath));
}

TEST_CASE("DocumentStore close is idempotent for unknown document") {
  DocumentStore store;
  const path filePath("/tmp/unknown.vel");

  REQUIRE_NOTHROW(store.close(filePath));
  REQUIRE_FALSE(store.has(filePath));
}
