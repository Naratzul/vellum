#include "pex_file.h"

#include <sstream>

#include "common/fs.h"
#include "pex_writer.h"

namespace vellum {
namespace pex {
void PexFile::writeToFile(std::string_view path) const {
  std::ostringstream stream(std::ios::binary);
  PexWriter writer(stream, getEndianness());

  writer << header_.magic << header_.majorVersion << header_.minorVersion
         << header_.gameID << header_.compilationTime
         << std::string_view(header_.sourceFile)
         << std::string_view(header_.userName)
         << std::string_view(header_.computerName);

  writer << stringTable_;

  if (hasDebugInfo()) {
    writer << true;
    writer << header_.gameID;
  } else {
    writer << false;
  }

  writer << userFlags_;

  writer << (uint16_t)objects_.size();
  for (const PexObject& object : objects_) {
    writer << object;
  }

  common::writeBinaryToFile(path, stream.str());
}

common::Endianness PexFile::getEndianness() const {
  return header().gameID == game::GameID::Skyrim ? common::Endianness::Big
                                                 : common::Endianness::Little;
}
}  // namespace pex
}  // namespace vellum