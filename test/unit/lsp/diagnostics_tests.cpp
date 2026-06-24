#include <catch2/catch_test_macros.hpp>

#include "diagnostics.h"
#include "lsp_fixture.h"
#include "lsp_locations.h"

using namespace vellum;
using vellum::lsp_test::LspTestFixture;

namespace {

constexpr const char* kValidScript = R"(script TrainingMannequin {
  var itemTypeBow = 7
  fun onHit() {
    itemTypeBow
  }
}
)";

constexpr const char* kParseErrorScript = R"(script TrainingMannequin {
  var itemTypeBow = 7
  fun onHit() {
    itemTypeBow
  }
)";

constexpr const char* kUndefinedIdentifierScript = R"(script TrainingMannequin {
  fun onHit() {
    unknownVar
  }
}
)";

const lsp::RelatedFullDocumentDiagnosticReport* asFullReport(
    const lsp::requests::TextDocument_Diagnostic::Result& result) {
  return std::get_if<lsp::RelatedFullDocumentDiagnosticReport>(&result);
}

bool sameRange(const lsp::Range& a, const lsp::Range& b) {
  return a.start.line == b.start.line && a.start.character == b.start.character &&
         a.end.line == b.end.line && a.end.character == b.end.character;
}

void requireMapsDiagnostics(const CachedAnalysis& cache,
                            const lsp::RelatedFullDocumentDiagnosticReport& report) {
  REQUIRE(report.items.size() == cache.diagnostics.size());

  for (size_t i = 0; i < cache.diagnostics.size(); ++i) {
    const DiagnosticMessage& src = cache.diagnostics[i];
    const lsp::Diagnostic& diag = report.items[i];

    REQUIRE(sameRange(diag.range, toLspRange(src.token)));
    REQUIRE(diag.message == src.message);
    REQUIRE(diag.severity.has_value());
    REQUIRE(*diag.severity == lsp::DiagnosticSeverity::Error);
  }
}

}  // namespace

TEST_CASE_METHOD(LspTestFixture, "LspDiagnostics valid script returns empty report") {
  openDoc(kValidScript);
  const CachedAnalysis& cache = analyze();

  REQUIRE(cache.diagnostics.empty());

  const auto result = Diagnostics::fromCache(cache);
  const auto* report = asFullReport(result);
  REQUIRE(report != nullptr);
  REQUIRE(report->items.empty());
}

TEST_CASE_METHOD(LspTestFixture, "LspDiagnostics parse error maps to LSP diagnostics") {
  openDoc(kParseErrorScript);
  const CachedAnalysis& cache = analyze();

  REQUIRE_FALSE(cache.diagnostics.empty());

  const auto result = Diagnostics::fromCache(cache);
  const auto* report = asFullReport(result);
  REQUIRE(report != nullptr);
  requireMapsDiagnostics(cache, *report);
}

TEST_CASE_METHOD(LspTestFixture,
                 "LspDiagnostics undefined identifier maps message and range") {
  openDoc(kUndefinedIdentifierScript);
  const CachedAnalysis& cache = analyze();

  REQUIRE_FALSE(cache.diagnostics.empty());

  const auto result = Diagnostics::fromCache(cache);
  const auto* report = asFullReport(result);
  REQUIRE(report != nullptr);
  requireMapsDiagnostics(cache, *report);

  bool foundUndefined = false;
  for (const auto& diag : report->items) {
    if (diag.message.find("unknownVar") != std::string::npos) {
      foundUndefined = true;
      REQUIRE(diag.range.start.line == 2);
      break;
    }
  }
  REQUIRE(foundUndefined);
}
