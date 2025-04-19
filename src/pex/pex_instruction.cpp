#include "pex_instruction.h"

#include "pex_writer.h"

namespace vellum {
namespace pex {
PexWriter& operator<<(PexWriter& writer, const PexInstruction& instruction) {
  writer << (uint8_t)instruction.getOpCode();
  for (const auto& arg : instruction.getArgs()) {
    writer << arg;
  }

  switch (instruction.getOpCode()) {
    case PexOpCode::CallMethod:
    case PexOpCode::CallStatic: {
      writer << PexValue((int32_t)instruction.getVariadicArgs().size());
      for (const auto& arg : instruction.getVariadicArgs()) {
        writer << arg;
      }
    }
    default:
      assert(instruction.getVariadicArgs().empty());
  }

  return writer;
}
}  // namespace pex
}  // namespace vellum