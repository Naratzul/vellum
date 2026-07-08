#include "completions_provider.h"

#include "analyze/import_library.h"
#include "common/string_utils.h"
#include "completion_collectors.h"
#include "completion_context.h"
#include "completion_items.h"
#include "document_store.h"

namespace vellum {
namespace {

void dedupeCandidates(Vec<CompletionCandidate>& candidates) {
  common::Set<std::string> seen;
  Vec<CompletionCandidate> unique;
  unique.reserve(candidates.size());
  for (auto& candidate : candidates) {
    std::string key = std::string(common::normalizeToLower(candidate.label));
    if (!candidate.detail.empty()) {
      key += '\0';
      key += common::normalizeToLower(candidate.detail);
    }
    if (!seen.insert(key).second) {
      continue;
    }
    unique.push_back(std::move(candidate));
  }
  candidates = std::move(unique);
}

void filterByPrefix(Vec<CompletionCandidate>& candidates,
                    std::string_view prefix) {
  if (prefix.empty()) {
    return;
  }
  Vec<CompletionCandidate> filtered;
  filtered.reserve(candidates.size());
  for (auto& candidate : candidates) {
    if (startsWithIgnoreCase(candidate.label, prefix)) {
      filtered.push_back(std::move(candidate));
    }
  }
  candidates = std::move(filtered);
}

}  // namespace

lsp::OneOf<lsp::Array<lsp::CompletionItem>, lsp::CompletionList>
CompletionsProvider::getCompletions(
    const path& filePath,
    const lsp::requests::TextDocument_Completion::Params& params,
    DocumentStore& store, const Shared<ImportLibrary>& importLibrary) {
  if (!store.has(filePath)) {
    return lsp::CompletionList{};
  }

  const std::string_view source = store.text(filePath);
  const CachedAnalysis& cache = store.getOrAnalyze(filePath, importLibrary);
  const PrefixAtCursor prefix = prefixAtPosition(source, params.position);
  const bool triggerDot =
      params.context &&
      params.context->triggerKind ==
          lsp::CompletionTriggerKind::TriggerCharacter &&
      params.context->triggerCharacter == ".";
  const CompletionContext ctx =
      detectContext(source, params, cache, prefix);

  Vec<CompletionCandidate> candidates;

  const bool canUseAst = cache.navigation && cache.navigation->parseOk;
  const bool canUseResolver =
      canUseAst && cache.navigation->resolver != nullptr;

  switch (ctx.kind) {
    case CompletionContextKind::MemberAccess: {
      const bool afterDot = prefix.afterDot || triggerDot;
      const std::string_view sourceLine =
          lineAt(source, params.position.line);
      if (canUseResolver) {
        Opt<VellumIdentifier> memberFilter;
        if (!prefix.prefix.empty()) {
          memberFilter = VellumIdentifier(prefix.prefix);
        }
        collectTypeMembers(*cache.navigation, importLibrary, params.position,
                         sourceLine, afterDot, memberFilter, candidates);
      }
      if (afterDot && (!canUseResolver || candidates.empty())) {
        collectTypeMembersFromLine(importLibrary, sourceLine, params.position,
                                 afterDot, candidates,
                                 canUseAst ? &*cache.navigation : nullptr);
      }
      break;
    }
    case CompletionContextKind::Expression:
      if (canUseResolver) {
        collectLocalsInScope(*cache.navigation, params.position, candidates);
        collectScriptMembers(*cache.navigation, candidates);
        collectBareCallableCandidates(*cache.navigation, importLibrary,
                                      candidates);
      }
      collectImportModules(importLibrary, candidates);
      collectKeywords(ctx.kind, candidates);
      break;
    case CompletionContextKind::ImportModule:
      collectImportModules(importLibrary, candidates);
      break;
    case CompletionContextKind::ScriptName:
      collectImportModules(importLibrary, candidates);
      break;
    case CompletionContextKind::ParentType:
      collectImportModules(importLibrary, candidates);
      if (canUseAst) {
        collectTypes(*cache.navigation, importLibrary, candidates);
      }
      break;
    case CompletionContextKind::TypeAnnotation:
      collectImportModules(importLibrary, candidates);
      if (canUseAst) {
        collectTypes(*cache.navigation, importLibrary, candidates);
      }
      break;
    case CompletionContextKind::Keyword:
      collectKeywords(ctx.kind, candidates);
      collectImportModules(importLibrary, candidates);
      break;
  }

  filterByPrefix(candidates, prefix.prefix);
  dedupeCandidates(candidates);

  const bool isIncomplete = !canUseResolver && !prefix.prefix.empty() &&
                            ctx.kind != CompletionContextKind::Keyword;

  return toCompletionList(candidates, prefix, isIncomplete);
}

}  // namespace vellum
