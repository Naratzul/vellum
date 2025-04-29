#pragma once

#include <optional>
#include <vector>

#include "pex_function_parameter.h"
#include "pex_instruction.h"
#include "pex_string.h"
#include "pex_user_flag.h"

namespace vellum {
namespace pex {
class PexFunction {
 public:
  PexFunction(std::optional<PexString> name, PexString returnTypeName,
              PexString documentationString, PexUserFlags userFlags,
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

  std::optional<PexString> getName() const { return name; }
  PexString getReturnTypeName() const { return returnTypeName; }
  PexString getDocumentationString() const { return documentationString; }
  PexUserFlags getUserFlags() const { return PexUserFlags(); }

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
  std::optional<PexString> name;
  PexString returnTypeName;
  PexString documentationString;
  PexUserFlags userFlags;

  std::vector<PexFunctionParameter> parameters;
  std::vector<PexFunctionParameter> localVariables;
  std::vector<PexInstruction> instructions;
};

PexWriter& operator<<(PexWriter& writer, const PexFunction& fun);
}  // namespace pex
}  // namespace vellum