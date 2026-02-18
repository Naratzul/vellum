#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common/binary_writer.h"
#include "common/types.h"
#include "game/game_id.h"
#include "pex_debug_info.h"
#include "pex_object.h"
#include "pex_string.h"
#include "pex_string_table.h"
#include "pex_user_flag.h"

namespace vellum {
namespace pex {
using common::Vec;

constexpr uint32_t PEX_MAGIC_NUM = 0xFA57C0DE;
constexpr uint32_t PEX_MAGIC_NUM_BE = 0xDE57C0FA;

#pragma pack(push, 1)
struct PexHeader {
  uint32_t magic = PEX_MAGIC_NUM;
  uint8_t majorVersion = 3;
  uint8_t minorVersion = 2;
  game::GameID gameID = game::GameID::Skyrim;
  uint64_t compilationTime = 0;
  std::string sourceFile;
  std::string userName;
  std::string computerName;
};
#pragma pack(pop)

struct PexUserFlagTableEntry {
  PexString name;
  uint8_t value;
};

class PexFile {
 public:
  using Objects = Vec<PexObject>;
  using PexUserFlagTable = Vec<PexUserFlagTableEntry>;

  PexHeader& header() { return header_; }
  const PexHeader& header() const { return header_; }

  PexStringTable& stringTable() { return stringTable_; }
  const PexStringTable& stringTable() const { return stringTable_; }

  PexUserFlagTable& userFlags() { return userFlags_; }
  const PexUserFlagTable& userFlags() const { return userFlags_; }

  Objects& objects() { return objects_; }
  const Objects& objects() const { return objects_; }

  bool hasDebugInfo() const { return debugInfo_ != nullptr; }
  PexDebugInfo* debugInfo() { return debugInfo_.get(); }
  const PexDebugInfo* debugInfo() const { return debugInfo_.get(); }
  void setDebugInfo(std::unique_ptr<PexDebugInfo> info) {
    debugInfo_ = std::move(info);
  }

  void writeToFile(std::string_view path) const;

  PexString getString(std::string_view str) {
    return stringTable().lookup(str);
  }

 private:
  PexHeader header_;
  PexStringTable stringTable_;
  PexUserFlagTable userFlags_;
  Objects objects_;
  std::unique_ptr<PexDebugInfo> debugInfo_;

  common::Endianness getEndianness() const;
};
};  // namespace pex
};  // namespace vellum