#include "vellum_value.h"

#include "pex/pex_file.h"

namespace vellum {
std::optional<pex::PexValue> makePexValue(VellumValue value,
                                          pex::PexFile& file) {
  switch (value.getType()) {
    case VellumValueType::String: {
      const pex::PexString pexValue = file.getString(value.asString());
      return pex::PexValue(pexValue);
    }
    case VellumValueType::Identifier:
      return pex::PexValue(
          pex::PexIdentifier(file.getString(value.asIdentifier().getValue())));
    case VellumValueType::Int:
      return pex::PexValue(value.asInt());
    case VellumValueType::Float:
      return pex::PexValue(value.asFloat());
    case VellumValueType::Bool:
      return pex::PexValue(value.asBool());
    case VellumValueType::None:
      return pex::PexValue();
  }

  return std::nullopt;
}
}  // namespace vellum