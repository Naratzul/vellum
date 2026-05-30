#include "completion_items.h"

namespace vellum {

lsp::CompletionList toCompletionList(const Vec<CompletionCandidate>& candidates,
                                     const PrefixAtCursor& prefix,
                                     bool isIncomplete) {
  lsp::CompletionList list;
  list.isIncomplete = isIncomplete;
  list.items.reserve(candidates.size());

  for (const auto& candidate : candidates) {
    lsp::CompletionItem item;
    item.label = candidate.label;
    item.kind = candidate.kind;
    if (!candidate.detail.empty()) {
      item.detail = candidate.detail;
    }
    if (candidate.filterText) {
      item.filterText = *candidate.filterText;
    }
    if (candidate.sortText) {
      item.sortText = *candidate.sortText;
    }
    if (candidate.insertText) {
      item.insertText = *candidate.insertText;
      item.insertTextFormat = lsp::InsertTextFormat::Snippet;
    }
    item.textEdit = lsp::TextEdit{
        .range = prefix.replaceRange, .newText = candidate.label};
    list.items.push_back(std::move(item));
  }

  return list;
}

}  // namespace vellum
