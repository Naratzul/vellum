#include "pex_object.h"

namespace vellum {
namespace pex {

PexWriter& vellum::pex::operator<<(PexWriter& writer, const PexObject& object) {
  writer << object.getName() << object.getObjectSize() << object.getParentName()
         << object.getDocumentationString() << object.getUserFlags()
         << object.getAutoStateName();

  writer << (uint16_t)object.getVariables().size();
  writer << (uint16_t)object.getProperties().size();
  writer << (uint16_t)object.getStates().size();

  return writer;
}

}  // namespace pex
}  // namespace vellum