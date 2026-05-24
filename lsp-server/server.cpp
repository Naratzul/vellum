#include "server.h"

#include <unordered_map>

#include "analyze/import_library.h"
#include "common/fs.h"
#include "common/os.h"
#include "definitions_provider.h"
#include "diagnostics.h"
#include "semantic_tokens.h"

namespace {
using namespace vellum::common;

// helper type for the visitor #4
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

Vec<fs::path> parseImportPathsFromInit(const lsp::InitializeParams& params) {
  Vec<fs::path> out;
  if (!params.initializationOptions) {
    return out;
  }

  const auto& any = *params.initializationOptions;
  if (!any.isObject()) {
    return out;
  }

  const auto& obj = any.object();
  if (!obj.contains("importPaths")) {
    return out;
  }

  const auto& pathsAny = obj.get("importPaths");
  if (!pathsAny.isArray()) {
    return out;
  }

  for (const auto& entry : pathsAny.array()) {
    if (entry.isString()) {
      out.push_back(fs::path(entry.string()));
    }
  }

  return out;
}
}  // namespace

namespace vellum {
LspServer::LspServer()
    : connection(lsp::io::standardIO()), messageHandler(connection) {
  registerHandlers();
}

bool LspServer::run() {
  while (isRunning) {
    messageHandler.processIncomingMessages();
  }

  return isShutdownRequested;
}

void LspServer::registerHandlers() {
  messageHandler
      .add<lsp::requests::Initialize>(
          [this](lsp::requests::Initialize::Params&& params) {
            logMsg("Received Initialize message");

            auto importPaths = parseImportPathsFromInit(params);
            importPaths = dedupePathsPreserveOrder(importPaths);

            logMsg("Import paths:");
            for (const auto& p : importPaths) {
              logMsg("- {}", p.string());
            }

            importLibrary = makeShared<ImportLibrary>(std::move(importPaths));

            return lsp::requests::Initialize::Result{
                .capabilities =
                    {.positionEncoding = lsp::PositionEncodingKind::UTF16,
                     .textDocumentSync = lsp::TextDocumentSyncKind::Full,
                     .definitionProvider = lsp::DefinitionOptions{},
                     .semanticTokensProvider =
                         lsp::SemanticTokensOptions{
                             .legend =
                                 lsp::SemanticTokensLegend{
                                     .tokenTypes = {"keyword", "string",
                                                    "number", "operator",
                                                    "variable", "class", "type",
                                                    "function", "property",
                                                    "parameter"},
                                     .tokenModifiers = {"declaration",
                                                        "readonly", "static"},
                                 },
                             .full = true},
                     .diagnosticProvider = lsp::DiagnosticOptions{}},
                .serverInfo = lsp::InitializeResultServerInfo{
                    .name = "Vellum Language Server", .version = "0.1.0"}};
          })
      .add<lsp::notifications::TextDocument_DidOpen>(
          [this](lsp::notifications::TextDocument_DidOpen::Params&& params) {
            logMsg("Opened file: {}", params.textDocument.uri.path());
            documents[std::string(params.textDocument.uri.path())] =
                params.textDocument.text;
          })
      .add<lsp::notifications::TextDocument_DidChange>(
          [this](lsp::notifications::TextDocument_DidChange::Params&& params) {
            logMsg("Did change file: {}", params.textDocument.uri.path());

            for (const auto& change : params.contentChanges) {
              handleDocumentChange(params.textDocument.uri, change);
            }
          })
      .add<lsp::requests::TextDocument_Diagnostic>(
          [&](lsp::requests::TextDocument_Diagnostic::Params&& params) {
            logMsg("Diagnostics request for: {}",
                   params.textDocument.uri.path());

            if (!documents.contains(
                    std::string(params.textDocument.uri.path()))) {
              logMsg("Can't do diagnostics, text is not synced yet.");
              return lsp::requests::TextDocument_Diagnostic::Result{};
            }

            std::string path(params.textDocument.uri.path());

            return vellum::Diagnostics().getDiagnostics(
                filenameWithoutExt(path), documents.at(path), importLibrary);
          })
      .add<lsp::requests::TextDocument_SemanticTokens_Full>(
          [this](lsp::requests::TextDocument_SemanticTokens_Full::Params&&
                     params) {
            logMsg("Semantic tokens request for: {}",
                   params.textDocument.uri.path());

            const std::string path(params.textDocument.uri.path());
            if (!documents.contains(path)) {
              logMsg("Can't provide semantic tokens, text is not synced yet.");
              return lsp::TextDocument_SemanticTokens_FullResult{};
            }

            return lsp::TextDocument_SemanticTokens_FullResult{
                SemanticTokensBuilder().build(documents.at(path))};
          })
      .add<lsp::requests::TextDocument_Definition>(
          [this](lsp::requests::TextDocument_Definition::Params&& params) {
            logMsg("Definition request for: {}",
                   params.textDocument.uri.path());

            const std::string path(params.textDocument.uri.path());
            if (!documents.contains(path)) {
              logMsg("Can't provide semantic tokens, text is not synced yet.");
              return lsp::requests::TextDocument_Definition::Result{};
            }

            return lsp::requests::TextDocument_Definition::Result{
                DefinitionsProvider().getDefinitions(params.position)};
          });

  messageHandler.add<lsp::requests::Shutdown>([&]() {
    logMsg("Received Shutdown message");
    isShutdownRequested = true;
    return lsp::requests::Shutdown::Result{};
  });

  messageHandler.add<lsp::notifications::Exit>([&]() { isRunning = false; });
}
void LspServer::handleDocumentChange(
    const lsp::DocumentUri& uri,
    const lsp::TextDocumentContentChangeEvent& changeEvent) {
  std::visit(
      overloaded{
          [&](const lsp::TextDocumentContentChangeEvent_Text& text) {
            documents[std::string(uri.path())] = text.text;
          },
          [&](const lsp::TextDocumentContentChangeEvent_Range_Text& text) {}},
      changeEvent);
}
}  // namespace vellum