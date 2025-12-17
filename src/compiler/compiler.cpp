#include "compiler.h"

#include <filesystem>

#include "ast/decl/declaration.h"
#include "common/types.h"
#include "compiler_error_handler.h"
#include "pex/pex_file.h"
#include "pex_object_compiler.h"

namespace vellum {
using common::Shared;
using common::Unique;

Compiler::Compiler(Shared<CompilerErrorHandler> errorHandler)
    : errorHandler(errorHandler) {}

pex::PexFile Compiler::compile(
    const ScriptMetadata& metadata,
    const std::vector<Unique<ast::Declaration>>& declarations) {
  pex::PexFile file;
  fillHeader(file.header(), metadata);

  PexObjectCompiler objectCompiler(errorHandler, file);
  pex::PexObject object = objectCompiler.compile(declarations);
  file.objects().push_back(std::move(object));

  return file;
}

void Compiler::fillHeader(pex::PexHeader& header,
                          const ScriptMetadata& metadata) {
  header.magic = pex::PEX_MAGIC_NUM;
  setCompilerVersion(metadata.gameID, header.majorVersion, header.minorVersion);
  header.sourceFile = metadata.sourceFile;
  header.compilationTime = metadata.compilationTime;
  header.userName = metadata.userName;
  header.computerName = metadata.computerName;
}

void Compiler::setCompilerVersion(game::GameID gameID, uint8_t& major,
                                  uint8_t& minor) const {
  switch (gameID) {
    case game::GameID::Skyrim:
      major = 3;
      minor = 2;
      break;
    case game::GameID::Fallout4:
      major = 3;
      minor = 9;
      break;
    case game::GameID::Fallout76:
      major = 3;
      minor = 15;
      break;
    case game::GameID::Starfield:
      major = 3;
      minor = 12;
      break;
  }
}

}  // namespace vellum
