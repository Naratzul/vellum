// Smoke test for completion (run: vellum-lsp-completion-smoke from build dir).
#include <iostream>
#include <sstream>
#include <string>

#include "analyze/import_library.h"
#include "common/fs.h"
#include "completion_context.h"
#include "completions_provider.h"
#include "document_store.h"
#include "lsp_locations.h"

using namespace vellum;

static bool hasLabel(const lsp::CompletionList& list, std::string_view label) {
  for (const auto& item : list.items) {
    if (item.label == label) {
      return true;
    }
  }
  return false;
}

static lsp::CompletionList getCompletionsAt(DocumentStore& store,
                                            const common::fs::path& filePath,
                                            const Shared<ImportLibrary>& lib,
                                            unsigned line,
                                            unsigned character) {
  lsp::requests::TextDocument_Completion::Params params;
  params.textDocument.uri = pathToUri(filePath);
  params.position = lsp::Position{.line = line, .character = character};
  const auto result =
      CompletionsProvider().getCompletions(filePath, params, store, lib);
  if (const auto* list = std::get_if<lsp::CompletionList>(&result)) {
    return *list;
  }
  return {};
}

int main() {
  static constexpr const char* kSource = R"(script TrainingMannequin {
	var itemTypeBow = 7
	var skillMarksman = "Marksman"

	fun onHit() {
		var player = 0
		itemTypeBow
	}
}
)";

  const common::fs::path filePath = "/tmp/TrainingMannequin.vel";
  auto importLibrary = common::makeShared<ImportLibrary>(common::Vec<common::fs::path>{});
  DocumentStore store;
  store.openOrUpdate(filePath, kSource);

  const auto& cache = store.getOrAnalyze(filePath, importLibrary);
  if (!cache.navigation || !cache.navigation->semanticOk) {
    std::cerr << "Expected semantic analysis to succeed for minimal fixture\n";
    for (const auto& diag : cache.diagnostics) {
      std::cerr << "  diag: " << diag.message << "\n";
    }
    return 1;
  }

  // Line 6: `		itemTypeBow` — prefix `itemTyp` at column 9
  const auto exprList = getCompletionsAt(store, filePath, importLibrary, 6, 9);
  std::cout << "Items in onHit: " << exprList.items.size() << "\n";
  for (const auto& item : exprList.items) {
    std::cout << "  - " << item.label << "\n";
  }

  const auto kwList = getCompletionsAt(store, filePath, importLibrary, 5, 2);
  std::cout << "Items for 'v' prefix: " << kwList.items.size() << "\n";

  int failures = 0;
  if (!hasLabel(exprList, "itemTypeBow")) {
    std::cerr << "Missing itemTypeBow in expression context\n";
    failures++;
  }
  if (!hasLabel(kwList, "var")) {
    std::cerr << "Missing keyword var\n";
    failures++;
  }

  static constexpr const char* kSourceWithUndefinedAdv = R"(script TrainingMannequin {
	var itemTypeBow = 7
	fun advanceSkill(skill: String, gain: Float) {}
	fun onHit() {
		adv
	}
}
)";

  store.openOrUpdate(filePath, kSourceWithUndefinedAdv);
  const auto& cacheWithAdv = store.getOrAnalyze(filePath, importLibrary);
  if (!cacheWithAdv.navigation || !cacheWithAdv.navigation->semanticOk) {
    std::cerr << "Expected best-effort semanticOk with undefined adv\n";
    failures++;
  }
  if (cacheWithAdv.diagnostics.empty()) {
    std::cerr << "Expected undefined identifier diagnostic for adv\n";
    failures++;
  }

  // Line 4: `		adv` — cursor after "adv"
  const auto advList = getCompletionsAt(store, filePath, importLibrary, 4, 5);
  if (!hasLabel(advList, "advanceSkill")) {
    std::cerr << "Expected advanceSkill while typing undefined adv\n";
    failures++;
  }

  const auto advSpaceList = getCompletionsAt(store, filePath, importLibrary, 4, 6);
  if (!hasLabel(advSpaceList, "advanceSkill")) {
    std::cerr << "Expected advanceSkill with trailing space after adv\n";
    failures++;
  }

  const common::fs::path examplesPath =
      common::fs::path(__FILE__).parent_path().parent_path() / "examples";
  auto importWithExamples = common::makeShared<ImportLibrary>(
      common::Vec<common::fs::path>{examplesPath});

  static constexpr const char* kUsesMathEx = R"(script TrainingMannequin {
  fun onHit() {
    Math
  }
}
)";

  store.openOrUpdate(filePath, kUsesMathEx);
  store.getOrAnalyze(filePath, importWithExamples);

  // Line 2: `    Math` — prefix `Math` at end of identifier
  const auto mathList =
      getCompletionsAt(store, filePath, importWithExamples, 2, 7);
  if (!hasLabel(mathList, "MathEx")) {
    std::cerr << "Expected MathEx when completing prefix Math\n";
    failures++;
  }

  static constexpr const char* kUsesMathDot = R"(script TrainingMannequin {
  fun onHit() {
    Math.
  }
}
)";

  store.openOrUpdate(filePath, kUsesMathDot);
  store.getOrAnalyze(filePath, importWithExamples);

  lsp::requests::TextDocument_Completion::Params dotParams;
  dotParams.textDocument.uri = pathToUri(filePath);
  dotParams.position = lsp::Position{.line = 2, .character = 8};
  dotParams.context = lsp::CompletionContext{
      .triggerKind = lsp::CompletionTriggerKind::TriggerCharacter,
      .triggerCharacter = "."};
  const auto dotResult = CompletionsProvider().getCompletions(
      filePath, dotParams, store, importWithExamples);
  const auto* dotList = std::get_if<lsp::CompletionList>(&dotResult);
  // examples/ has MathEx but not Math; incomplete exact match still resolves to MathEx.
  if (!dotList || !hasLabel(*dotList, "chance")) {
    std::cerr << "Expected MathEx.chance when only MathEx is on the import path\n";
    failures++;
  }

  const common::fs::path scriptsPath =
      common::fs::path(__FILE__).parent_path().parent_path().parent_path() /
      "Scripts" / "Source" / "Scripts";
  if (common::fs::exists(scriptsPath / "Math.psc")) {
    auto importWithMathAndMathEx = common::makeShared<ImportLibrary>(
        common::Vec<common::fs::path>{examplesPath, scriptsPath});
    store.openOrUpdate(filePath, kUsesMathDot);
    store.getOrAnalyze(filePath, importWithMathAndMathEx);

    const auto mathDotResult = CompletionsProvider().getCompletions(
        filePath, dotParams, store, importWithMathAndMathEx);
    const auto* mathDotList =
        std::get_if<lsp::CompletionList>(&mathDotResult);
    if (!mathDotList || !hasLabel(*mathDotList, "abs")) {
      std::cerr << "Expected Math.abs when Math.psc is on the import path\n";
      failures++;
    }
    if (mathDotList && hasLabel(*mathDotList, "chance")) {
      std::cerr << "Math. must not include MathEx.chance when Math.psc exists\n";
      failures++;
    }
  }

  return failures == 0 ? 0 : 1;
}
