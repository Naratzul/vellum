#pragma once

namespace vellum {
  namespace pex {
    enum class OpCode : uint8_t
    {
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
  };
};