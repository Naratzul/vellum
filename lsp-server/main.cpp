#include <lsp/connection.h>
#include <lsp/io/standardio.h>
#include <lsp/messagehandler.h>
#include <lsp/messages.h>  // Generated message definitions

#include <format>

auto connection = lsp::Connection(lsp::io::standardIO());
auto messageHandler = lsp::MessageHandler(connection);

void logMsg(const std::string& msg) {
  lsp::LogMessageParams params{.type = lsp::MessageType::Info, .message = msg};
  messageHandler.sendNotification<lsp::notifications::Window_LogMessage>(
      std::move(params));
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
          })
      .add<lsp::notifications::TextDocument_DidChange>(
          [](lsp::notifications::TextDocument_DidChange::Params&& params) {
            logMsg(std::format("Did change file: {}",
                               params.textDocument.uri.path()));
          })
      .add<lsp::requests::TextDocument_Diagnostic>(
          [](lsp::requests::TextDocument_Diagnostic::Params&& params) {
            logMsg(std::format("Diagnostics request for: {}",
                               params.textDocument.uri.path()));

            lsp::RelatedFullDocumentDiagnosticReport result;
            result.items.push_back(lsp::Diagnostic{
                .range = {.start = lsp::Position{5, 1}, .end = lsp::Position{5, 5}},
                .message = "Test message",
                .severity = lsp::DiagnosticSeverity::Error});

            return lsp::DocumentDiagnosticReport{result};
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

  while (isRunning) {
    messageHandler.processIncomingMessages();
  }

  return isShutdownRequested ? 0 : 1;
}