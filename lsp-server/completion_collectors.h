#pragma once

#include <lsp/types.h>

#include <optional>
#include <string>
#include <vector>

#include "analyze/import_library.h"
#include "common/fs.h"
#include "common/types.h"
#include "completion_context.h"
#include "document_analyzer.h"
#include "vellum/vellum_identifier.h"

namespace vellum {

struct CompletionCandidate {
  std::string label;
  lsp::CompletionItemKind kind{lsp::CompletionItemKind::Text};
  std::string detail;
  std::optional<std::string> insertText;
  std::optional<std::string> filterText;
  std::optional<std::string> sortText;
};

bool startsWithIgnoreCase(std::string_view haystack, std::string_view prefix);

void collectKeywords(CompletionContextKind context,
                     Vec<CompletionCandidate>& out);

void collectImportModules(const Shared<ImportLibrary>& importLibrary,
                            Vec<CompletionCandidate>& out);

void collectLocalsInScope(const NavigationContext& navigation, lsp::Position pos,
                          Vec<CompletionCandidate>& out);

void collectScriptMembers(const NavigationContext& navigation,
                          Vec<CompletionCandidate>& out);

void collectTypeMembers(const NavigationContext& navigation,
                        const Shared<ImportLibrary>& importLibrary,
                        lsp::Position pos, std::string_view sourceLine,
                        bool afterDot, Opt<VellumIdentifier> memberPrefix,
                        Vec<CompletionCandidate>& out);

// When the document does not parse (e.g. trailing `.`), complete import modules
// from the identifier text before the dot.
void collectTypeMembersFromLine(const Shared<ImportLibrary>& importLibrary,
                                std::string_view sourceLine, lsp::Position pos,
                                bool afterDot, Vec<CompletionCandidate>& out);

void collectTypes(const NavigationContext& navigation,
                  const Shared<ImportLibrary>& importLibrary,
                  Vec<CompletionCandidate>& out);

}  // namespace vellum
