#include "server.h"

#include "analyze/import_library.h"
#include "common/fs.h"
#include "common/os.h"
#include "definitions_provider.h"
#include "diagnostics.h"

namespace {
using namespace vellum::common;

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

Vec<fs::path> parseImportPathsFromArray(const lsp::json::Any& pathsAny) {
  Vec<fs::path> out;
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

Vec<fs::path> parseImportPathsFromInit(const lsp::InitializeParams& params) {
  if (!params.initializationOptions) {
    return {};
  }

  const auto& any = *params.initializationOptions;
  if (!any.isObject()) {
    return {};
  }

  const auto& obj = any.object();
  if (!obj.contains("importPaths")) {
    return {};
  }

  return parseImportPathsFromArray(obj.get("importPaths"));
}

Vec<fs::path> parseImportPathsFromConfiguration(
    const lsp::DidChangeConfigurationParams& params) {
  if (!params.settings.isObject()) {
    return {};
  }

  const auto& settings = params.settings.object();
  if (!settings.contains("vellum")) {
    return {};
  }

  const auto& vellumAny = settings.get("vellum");
  if (!vellumAny.isObject()) {
    return {};
  }

  const auto& vellum = vellumAny.object();
  if (!vellum.contains("importPaths")) {
    return {};
  }

  return parseImportPathsFromArray(vellum.get("importPaths"));
}
}  // namespace

namespace vellum {
using common::fs::path;
using common::makeShared;

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

void LspServer::applyImportPaths(Vec<path> importPaths) {
  importPaths = dedupePathsPreserveOrder(importPaths);

  logMsg("Import paths:");
  for (const auto& p : importPaths) {
    logMsg("- {}", p.string());
  }

  importLibrary = makeShared<ImportLibrary>(std::move(importPaths));
  documentStore.invalidateAll();
}

void LspServer::requestDiagnosticAndSemanticRefresh() {
  messageHandler.sendRequest<lsp::requests::Workspace_Diagnostic_Refresh>(
      [](std::nullptr_t) {},
      [this](const lsp::ResponseError& error) {
        logMsg("workspace/diagnostic/refresh failed: {}", error.message());
      });

  messageHandler.sendRequest<lsp::requests::Workspace_SemanticTokens_Refresh>(
      [](std::nullptr_t) {},
      [this](const lsp::ResponseError& error) {
        logMsg("workspace/semanticTokens/refresh failed: {}", error.message());
      });
}

void LspServer::registerHandlers() {
  messageHandler
      .add<lsp::requests::Initialize>(
          [this](lsp::requests::Initialize::Params&& params) {
            logMsg("Received Initialize message");
            applyImportPaths(parseImportPathsFromInit(params));

            return lsp::requests::Initialize::Result{
                .capabilities =
                    {.positionEncoding = lsp::PositionEncodingKind::UTF16,
                     .textDocumentSync = lsp::TextDocumentSyncKind::Full,
                     .definitionProvider =
                         lsp::OneOf<bool, lsp::DefinitionOptions>(true),
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
      .add<lsp::notifications::Workspace_DidChangeConfiguration>(
          [this](lsp::notifications::Workspace_DidChangeConfiguration::Params&&
                     params) {
            logMsg("Received workspace/didChangeConfiguration");
            applyImportPaths(parseImportPathsFromConfiguration(params));
            requestDiagnosticAndSemanticRefresh();
          })
      .add<lsp::notifications::TextDocument_DidOpen>(
          [this](lsp::notifications::TextDocument_DidOpen::Params&& params) {
            const path filePath = pathFromDocumentUri(params.textDocument.uri);
            logMsg("Opened file: {}", filePath.string());
            documentStore.openOrUpdate(filePath, params.textDocument.text);
          })
      .add<lsp::notifications::TextDocument_DidChange>(
          [this](lsp::notifications::TextDocument_DidChange::Params&& params) {
            logMsg("Did change file: {}", params.textDocument.uri.path());

            for (const auto& change : params.contentChanges) {
              handleDocumentChange(params.textDocument.uri, change);
            }
          })
      .add<lsp::requests::TextDocument_Diagnostic>(
          [this](lsp::requests::TextDocument_Diagnostic::Params&& params) {
            const path filePath = pathFromDocumentUri(params.textDocument.uri);
            logMsg("Diagnostics request for: {}", filePath.string());

            if (!documentStore.has(filePath)) {
              logMsg("Can't do diagnostics, text is not synced yet.");
              return lsp::requests::TextDocument_Diagnostic::Result{};
            }

            const CachedAnalysis& analysis =
                documentStore.getOrAnalyze(filePath, importLibrary);
            return Diagnostics::fromCache(analysis);
          })
      .add<lsp::requests::TextDocument_SemanticTokens_Full>(
          [this](lsp::requests::TextDocument_SemanticTokens_Full::Params&&
                     params) {
            const path filePath = pathFromDocumentUri(params.textDocument.uri);
            logMsg("Semantic tokens request for: {}", filePath.string());

            if (!documentStore.has(filePath)) {
              logMsg("Can't provide semantic tokens, text is not synced yet.");
              return lsp::TextDocument_SemanticTokens_FullResult{};
            }

            const CachedAnalysis& analysis =
                documentStore.getOrAnalyze(filePath, importLibrary);
            return lsp::TextDocument_SemanticTokens_FullResult{
                analysis.semanticTokens};
          })
      .add<lsp::requests::TextDocument_Definition>(
          [this](lsp::requests::TextDocument_Definition::Params&& params) {
            const path filePath = pathFromDocumentUri(params.textDocument.uri);
            logMsg("Definition request for: {}", filePath.string());

            if (!documentStore.has(filePath)) {
              logMsg("Can't provide definition, text is not synced yet.");
              return lsp::requests::TextDocument_Definition::Result{};
            }

            return lsp::requests::TextDocument_Definition::Result{
                DefinitionsProvider().getDefinitions(
                    filePath, documentStore.scriptName(filePath), params.position,
                    documentStore, importLibrary)};
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
            const path filePath = pathFromDocumentUri(uri);
            documentStore.openOrUpdate(filePath, text.text);
          },
          [&](const lsp::TextDocumentContentChangeEvent_Range_Text& text) {}},
      changeEvent);
}
}  // namespace vellum
