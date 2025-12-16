#include <lsp/connection.h>
#include <lsp/io/standardio.h>
#include <lsp/messagehandler.h>
#include <lsp/messages.h>  // Generated message definitions

#include <format>
#include <unordered_map>

#include "common/os.h"
#include "diagnostics.h"

auto connection = lsp::Connection(lsp::io::standardIO());
auto messageHandler = lsp::MessageHandler(connection);

std::unordered_map<std::string, std::string> documents;

void logMsg(const std::string& msg) {
  lsp::LogMessageParams params{.type = lsp::MessageType::Info, .message = msg};
  messageHandler.sendNotification<lsp::notifications::Window_LogMessage>(
      std::move(params));
}

// helper type for the visitor #4
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void handleDocumentChange(
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

int main() {
  messageHandler
      .add<lsp::requests::Initialize>(
          [](lsp::requests::Initialize::Params&& params) {
            logMsg("Received Initialize message");

            return lsp::requests::Initialize::Result{
                .capabilities =
                    {.positionEncoding = lsp::PositionEncodingKind::UTF16,
                     .textDocumentSync = lsp::TextDocumentSyncKind::Full,
                     .diagnosticProvider = lsp::DiagnosticOptions{}},
                .serverInfo = lsp::InitializeResultServerInfo{
                    .name = "Language Server", .version = "1.0.0"}};
          })
      .add<lsp::notifications::TextDocument_DidOpen>(
          [](lsp::notifications::TextDocument_DidOpen::Params&& params) {
            logMsg(
                std::format("Opened file: {}", params.textDocument.uri.path()));
            documents[std::string(params.textDocument.uri.path())] =
                params.textDocument.text;
          })
      .add<lsp::notifications::TextDocument_DidChange>(
          [](lsp::notifications::TextDocument_DidChange::Params&& params) {
            logMsg(std::format("Did change file: {}",
                               params.textDocument.uri.path()));

            for (const auto& change : params.contentChanges) {
              handleDocumentChange(params.textDocument.uri, change);
            }
          })
      .add<lsp::requests::TextDocument_Diagnostic>(
          [](lsp::requests::TextDocument_Diagnostic::Params&& params) {
            logMsg(std::format("Diagnostics request for: {}",
                               params.textDocument.uri.path()));

            if (!documents.contains(
                    std::string(params.textDocument.uri.path()))) {
              logMsg("Can't do diagnostics, text is not synced yet.");
              return lsp::requests::TextDocument_Diagnostic::Result{};
            }

            return vellum::Diagnostics().getDiagnostics(
                documents.at(std::string(params.textDocument.uri.path())));
          });

  bool isShutdownRequested = false;
  messageHandler.add<lsp::requests::Shutdown>([&isShutdownRequested]() {
    logMsg("Received Shutdown message");
    isShutdownRequested = true;
    return lsp::requests::Shutdown::Result{};
  });

  bool isRunning = true;
  messageHandler.add<lsp::notifications::Exit>(
      [&isRunning]() { isRunning = false; });

  logMsg("Hello from server");

  while (!vellum::common::isDebuggerPresent()) {
  }

  while (isRunning) {
    messageHandler.processIncomingMessages();
  }

  return isShutdownRequested ? 0 : 1;
}