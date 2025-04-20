#pragma once

#include <memory>
#include <string>
#include <vector>

#include "game/game_id.h"

namespace vellum {

class CompilerErrorHandler;

namespace pex {
class PexFile;
struct PexHeader;
class PexObject;
}  // namespace pex

namespace ast {
class Declaration;
class Statement;
}  // namespace ast

struct ScriptMetadata {
  game::GameID gameID = game::GameID::Skyrim;
  uint64_t compilationTime = 0;
  std::string sourceFile;
  std::string userName;
  std::string computerName;
};

class Compiler {
 public:
  explicit Compiler(std::shared_ptr<CompilerErrorHandler> errorHandler);

  pex::PexFile compile(
      const ScriptMetadata& metadata,
      const std::vector<std::unique_ptr<ast::Declaration>>& declarations);

 private:
  std::shared_ptr<CompilerErrorHandler> errorHandler;

  void fillHeader(pex::PexHeader& header, const ScriptMetadata& metadata);
  void setCompilerVersion(game::GameID gameID, uint8_t& major,
                          uint8_t& minor) const;
};
}  // namespace vellum