#pragma once

#include <optional>
#include <vector>

#include "common/types.h"
#include "pex_function_parameter.h"
#include "pex_instruction.h"
#include "pex_string.h"
#include "pex_user_flag.h"

namespace vellum {
namespace pex {
using common::Opt;
using common::Vec;
class PexFunction {
 public:
  PexFunction(Opt<PexString> name, PexString returnTypeName,
              PexString documentationString, PexUserFlags userFlags,
              Vec<PexFunctionParameter> parameters,
              Vec<PexFunctionParameter> localVariables,
              Vec<PexInstruction> instructions)
      : name(name),
        returnTypeName(returnTypeName),
        documentationString(documentationString),
        userFlags(userFlags),
        parameters(std::move(parameters)),
        localVariables(std::move(localVariables)),
        instructions(std::move(instructions)) {}

  Opt<PexString> getName() const { return name; }
  PexString getReturnTypeName() const { return returnTypeName; }
  PexString getDocumentationString() const { return documentationString; }
  PexUserFlags getUserFlags() const { return PexUserFlags(); }

  const Vec<PexFunctionParameter>& getParameters() const { return parameters; }

  const Vec<PexFunctionParameter>& getLocalVariables() const {
    return localVariables;
  }

  const Vec<PexInstruction>& getInstructions() const { return instructions; }

 private:
  Opt<PexString> name;
  PexString returnTypeName;
  PexString documentationString;
  PexUserFlags userFlags;

  Vec<PexFunctionParameter> parameters;
  Vec<PexFunctionParameter> localVariables;
  Vec<PexInstruction> instructions;
};

PexWriter& operator<<(PexWriter& writer, const PexFunction& fun);
}  // namespace pex
}  // namespace vellum