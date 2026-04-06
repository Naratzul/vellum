#include "pex_file.h"

#include <sstream>

#include "common/fs.h"
#include "pex_debug_info.h"
#include "pex_writer.h"

namespace vellum {
namespace pex {
void PexFile::writeToFile(const fs::path& path) const {
  std::ostringstream stream(std::ios::binary);
  PexWriter writer(stream, getEndianness());

  writer << header_.magic << header_.majorVersion << header_.minorVersion
         << header_.gameID << header_.compilationTime
         << std::string_view(header_.sourceFile)
         << std::string_view(header_.userName)
         << std::string_view(header_.computerName);

  writer << stringTable_;

  writer << static_cast<uint8_t>(hasDebugInfo() ? 1 : 0);
  if (hasDebugInfo()) {
    write(*debugInfo_, writer);
  }

  writer << (uint16_t)userFlags_.size();
  for (const auto& entry : userFlags_) {
    writer << entry.name << entry.value;
  }

  writer << objects_;

  common::writeBinaryToFile(path, stream.str());
}

common::Endianness PexFile::getEndianness() const {
  return header().gameID == game::GameID::Skyrim ? common::Endianness::Big
                                                 : common::Endianness::Little;
}
}  // namespace pex
}  // namespace vellum