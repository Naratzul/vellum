#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common/types.h"
#include "game/game_id.h"

namespace vellum {
using common::Shared;
using common::Unique;
using common::Vec;

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
  Compiler(Shared<CompilerErrorHandler> errorHandler);

  pex::PexFile compile(
      const ScriptMetadata& metadata,
      const Vec<Unique<ast::Declaration>>& declarations);

 private:
  Shared<CompilerErrorHandler> errorHandler;

  void fillHeader(pex::PexHeader& header, const ScriptMetadata& metadata);
  void setCompilerVersion(game::GameID gameID, uint8_t& major,
                          uint8_t& minor) const;
};
}  // namespace vellum