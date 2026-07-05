#include <lsp/connection.h>
#include <lsp/io/standardio.h>
#include <lsp/messagehandler.h>
#include <lsp/messages.h>  // Generated message definitions

#include <format>

#include "common/fs.h"
#include "common/types.h"
#include "document_store.h"

namespace vellum {
using common::Shared;
using common::Vec;
using common::fs::path;

class LspServer {
 public:
  LspServer();

  bool run();

 private:
  bool isRunning{true};
  bool isShutdownRequested{false};

  lsp::Connection connection;
  lsp::MessageHandler messageHandler;
  DocumentStore documentStore;
  Shared<ImportLibrary> importLibrary{nullptr};
  common::Opt<path> outputDirectory{std::nullopt};

  void registerHandlers();

  void applyImportPaths(Vec<path> importPaths);
  void applyOutputDirectory(common::Opt<path> outputDirectory);
  void requestDiagnosticAndSemanticRefresh();

  void handleDocumentChange(
      const lsp::DocumentUri& uri,
      const lsp::TextDocumentContentChangeEvent& changeEvent);

  template <typename... Types>
  void logMsg(const std::format_string<Types...> fmt, Types&&... args);
};

template <typename... Types>
void LspServer::logMsg(const std::format_string<Types...> fmt,
                       Types&&... args) {
  lsp::LogMessageParams params{
      .type = lsp::MessageType::Info,
      .message = std::format(fmt, std::forward<Types>(args)...)};
  messageHandler.sendNotification<lsp::notifications::Window_LogMessage>(
      std::move(params));
}

}  // namespace vellum