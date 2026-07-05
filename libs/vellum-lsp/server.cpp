#include "server.h"

#ifndef VELLUM_LSP_VERSION
#error VELLUM_LSP_VERSION must be defined by CMake (apps/vellum-lsp project VERSION)
#endif

#include "analyze/import_library.h"
#include "common/fs.h"
#include "common/os.h"
#include "completions_provider.h"
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

Opt<fs::path> parseOutputDirectoryFromString(
    const lsp::json::Any& pathAny) {
  if (!pathAny.isString()) {
    return std::nullopt;
  }

  const auto& value = pathAny.string();
  if (value.empty()) {
    return std::nullopt;
  }

  return fs::path(value);
}

struct VellumSettings {
  Vec<fs::path> importPaths;
  Opt<fs::path> outputDirectory;
};

VellumSettings parseVellumSettingsFromObject(const lsp::json::Object& obj) {
  VellumSettings settings;
  if (obj.contains("importPaths")) {
    settings.importPaths = parseImportPathsFromArray(obj.get("importPaths"));
  }
  if (obj.contains("outputDirectory")) {
    settings.outputDirectory =
        parseOutputDirectoryFromString(obj.get("outputDirectory"));
  }
  return settings;
}

VellumSettings parseVellumSettingsFromInit(const lsp::InitializeParams& params) {
  if (!params.initializationOptions) {
    return {};
  }

  const auto& any = *params.initializationOptions;
  if (!any.isObject()) {
    return {};
  }

  return parseVellumSettingsFromObject(any.object());
}

VellumSettings parseVellumSettingsFromConfiguration(
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

  return parseVellumSettingsFromObject(vellumAny.object());
}
}  // namespace

namespace vellum {
using common::fs::path;
using common::makeShared;
using common::Opt;

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

void LspServer::applyOutputDirectory(Opt<path> newOutputDirectory) {
  outputDirectory = std::move(newOutputDirectory);

  if (outputDirectory) {
    logMsg("Output directory: {}", outputDirectory->string());
  } else {
    logMsg("Output directory: (next to source file)");
  }
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
            const VellumSettings settings = parseVellumSettingsFromInit(params);
            applyImportPaths(settings.importPaths);
            applyOutputDirectory(settings.outputDirectory);

            return lsp::requests::Initialize::Result{
                .capabilities =
                    {.positionEncoding = lsp::PositionEncodingKind::UTF16,
                     .textDocumentSync = lsp::TextDocumentSyncKind::Full,
                     .completionProvider =
                         lsp::CompletionOptions{
                             .triggerCharacters =
                                 lsp::Array<lsp::String>{".", ":", "\""},
                             .resolveProvider = false},
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
                    .name = "Vellum Language Server",
                    .version = VELLUM_LSP_VERSION}};
          })
      .add<lsp::notifications::Workspace_DidChangeConfiguration>(
          [this](lsp::notifications::Workspace_DidChangeConfiguration::Params&&
                     params) {
            logMsg("Received workspace/didChangeConfiguration");
            const VellumSettings settings =
                parseVellumSettingsFromConfiguration(params);
            applyImportPaths(settings.importPaths);
            applyOutputDirectory(settings.outputDirectory);
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
      .add<lsp::notifications::TextDocument_DidClose>(
          [this](lsp::notifications::TextDocument_DidClose::Params&& params) {
            const path filePath = pathFromDocumentUri(params.textDocument.uri);
            logMsg("Closed file: {}", filePath.string());
            documentStore.close(filePath);
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
                    filePath, params.position, documentStore, importLibrary)};
          })
      .add<lsp::requests::TextDocument_Completion>(
          [this](lsp::requests::TextDocument_Completion::Params&& params) {
            const path filePath = pathFromDocumentUri(params.textDocument.uri);
            logMsg("Completion request for: {}", filePath.string());

            if (!documentStore.has(filePath)) {
              logMsg("Can't provide completion, text is not synced yet.");
              return lsp::requests::TextDocument_Completion::Result{};
            }

            return lsp::requests::TextDocument_Completion::Result{
                CompletionsProvider().getCompletions(
                    filePath, params, documentStore, importLibrary)};
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
          [&](const lsp::TextDocumentContentChangeEvent_Range_Text& text) {
            const path filePath = pathFromDocumentUri(uri);
            documentStore.applyChange(filePath, text.range, text.text);
          }},
      changeEvent);
}
}  // namespace vellum
