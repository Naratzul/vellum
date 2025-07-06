#include <lsp/connection.h>
#include <lsp/io/standardio.h>
#include <lsp/messagehandler.h>
#include <lsp/messages.h>  // Generated message definitions

#include <format>

auto connection = lsp::Connection(lsp::io::standardIO());
auto messageHandler = lsp::MessageHandler(connection);

void logMsg(const std::string& msg) {
  lsp::LogMessageParams params{.message = msg, .type = lsp::MessageType::Info};
  messageHandler.sendNotification<lsp::notifications::Window_LogMessage>(
      std::move(params));
}

int main() {
  messageHandler.add<lsp::requests::Initialize>(
      [](lsp::requests::Initialize::Params&& params) {
        logMsg("Received Initialize message");

        return lsp::requests::Initialize::Result{
            .serverInfo =
                lsp::InitializeResultServerInfo{.name = "Language Server",
                                                .version = "1.0.0"},
            .capabilities = {.positionEncoding =
                                 lsp::PositionEncodingKind::UTF16}};
      });

  messageHandler.add<lsp::notifications::TextDocument_DidOpen>(
      [](lsp::notifications::TextDocument_DidOpen::Params&& params) {
        logMsg(std::format("Opened file: {}", params.textDocument.uri.path()));
      });

  messageHandler.add<lsp::notifications::TextDocument_DidChange>(
      [](lsp::notifications::TextDocument_DidChange&& params) {

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