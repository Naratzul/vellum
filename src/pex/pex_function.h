#pragma once

#include <vector>

#include "pex_function_parameter.h"
#include "pex_instruction.h"
#include "pex_string.h"
#include "pex_user_flag.h"

namespace vellum {
namespace pex {
class PexFunction {
 public:
  PexFunction(PexString name, PexString returnTypeName,
              PexString documentationString, UserFlags userFlags,
              std::vector<PexFunctionParameter> parameters,
              std::vector<PexFunctionParameter> localVariables,
              std::vector<PexInstruction> instructions)
      : name(name),
        returnTypeName(returnTypeName),
        documentationString(documentationString),
        userFlags(userFlags),
        parameters(std::move(parameters)),
        localVariables(std::move(localVariables)),
        instructions(std::move(instructions)) {}

  PexString getName() const { return name; }
  PexString getReturnTypeName() const { return returnTypeName; }
  PexString getDocumentationString() const { return documentationString; }
  UserFlags getUserFlags() const { return userFlags; }

  const std::vector<PexFunctionParameter>& getParameters() const {
    return parameters;
  }

  const std::vector<PexFunctionParameter>& getLocalVariables() const {
    return localVariables;
  }

  const std::vector<PexInstruction>& getInstructions() const {
    return instructions;
  }

 private:
  PexString name;
  PexString returnTypeName;
  PexString documentationString;
  UserFlags userFlags;

  std::vector<PexFunctionParameter> parameters;
  std::vector<PexFunctionParameter> localVariables;
  std::vector<PexInstruction> instructions;
};

PexWriter& operator<<(PexWriter& writer, const PexFunction& fun);
}  // namespace pex
}  // namespace vellum