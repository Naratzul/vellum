#pragma once

#include <vector>

#include "pex_value.h"

namespace vellum {
namespace pex {

class PexWriter;

enum class PexOpCode : uint8_t {
  Invalid = 0xFF,

  Nop = 0,
  IAdd,
  FAdd,
  ISub,
  FSub,
  IMul,
  FMul,
  IDiv,
  FDiv,
  IMod,
  Not,
  INeg,
  FNeg,
  Assign,
  Cast,
  CmpEq,
  CmpLt,
  CmpLte,
  CmpGt,
  CmpGte,
  Jmp,
  JmpT,
  JmpF,
  CallMethod,
  CallParent,
  CallStatic,
  Return,
  StrCat,
  PropGet,
  PropSet,
  ArrayCreate,
  ArrayLength,
  ArrayGetElement,
  ArraySetElement,
  ArrayFindElement,
  ArrayRFindElement,
};

class PexInstruction {
 public:
  PexInstruction(PexOpCode code, std::vector<PexValue> args,
                 std::vector<PexValue> variadicArgs)
      : code(code),
        args(std::move(args)),
        variadicArgs(std::move(variadicArgs)) {}

  PexOpCode getOpCode() const { return code; }
  const std::vector<PexValue>& getArgs() const { return args; }
  const std::vector<PexValue>& getVariadicArgs() const { return variadicArgs; }

 private:
  PexOpCode code;
  std::vector<PexValue> args;
  std::vector<PexValue> variadicArgs;
};

PexWriter& operator<<(PexWriter& writer, const PexInstruction& instruction);
};  // namespace pex
};  // namespace vellum