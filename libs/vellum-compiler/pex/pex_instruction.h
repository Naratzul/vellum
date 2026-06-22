#pragma once

#include <vector>

#include "common/types.h"
#include "pex_value.h"

namespace vellum {
namespace pex {
using common::Vec;

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
  PexInstruction(PexOpCode code, Vec<PexValue> args,
                 Vec<PexValue> variadicArgs = {})
      : code(code),
        args(std::move(args)),
        variadicArgs(std::move(variadicArgs)) {}

  PexOpCode getOpCode() const { return code; }
  const Vec<PexValue>& getArgs() const { return args; }
  const Vec<PexValue>& getVariadicArgs() const { return variadicArgs; }

  void setArg(int index, pex::PexValue value) { args[index] = value; }

 private:
  PexOpCode code;
  Vec<PexValue> args;
  Vec<PexValue> variadicArgs;
};

PexWriter& operator<<(PexWriter& writer, const PexInstruction& instruction);
};  // namespace pex
};  // namespace vellum