#pragma once

#include <optional>

#include "common/types.h"
#include "pex_function.h"
#include "pex_string.h"
#include "pex_user_flag.h"

namespace vellum {
namespace pex {
using common::Opt;

class PexWriter;

class PexProperty {
 public:
  PexProperty(PexString name, PexString typeName, PexString documentationString,
              PexUserFlags userFlags, Opt<PexFunction> getAccessor,
              Opt<PexFunction> setAccessor, Opt<PexString> backedVariableName)
      : name(name),
        typeName(typeName),
        documentationString(documentationString),
        userFlags(userFlags),
        getAccessor(getAccessor),
        setAccessor(setAccessor),
        backedVariableName(backedVariableName) {}

  PexString getName() const { return name; }
  PexString getTypeName() const { return typeName; }
  PexString getDocumentationString() const { return documentationString; }
  PexUserFlags getUserFlags() const { return userFlags; }

  Opt<PexFunction> getGetAccessor() const { return getAccessor; }
  Opt<PexFunction> getSetAccessor() const { return setAccessor; }

  Opt<PexString> getBackedVariableName() const { return backedVariableName; }

 private:
  PexString name;
  PexString typeName;
  PexString documentationString;
  PexUserFlags userFlags;

  Opt<PexFunction> getAccessor;
  Opt<PexFunction> setAccessor;

  Opt<PexString> backedVariableName;
};

PexWriter& operator<<(PexWriter& writer, const PexProperty& property);
}  // namespace pex
}  // namespace vellum