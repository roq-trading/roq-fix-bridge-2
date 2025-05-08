/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/fix_bridge/fix_log.hpp"

namespace roq {
namespace fix_bridge {

// === HELPERS ===

namespace {
auto create_file(auto &path) -> std::unique_ptr<io::fs::File> {
  if (std::empty(path)) {
    return {};
  }
  auto flags = Mask{
      io::fs::Flags::WRITE_ONLY,
      io::fs::Flags::CREATE,
      io::fs::Flags::APPEND,
  };
  auto mode = Mask{
      io::fs::Mode::USER_READ_WRITE,
      io::fs::Mode::GROUP_READ_WRITE,
      io::fs::Mode::OTHERS_READ_WRITE,
  };
  return std::make_unique<io::fs::File>(path, flags, mode);
}
}  // namespace

// === IMPLEMENTATION ===

FIXLog::FIXLog(Settings const &settings) : settings_{settings}, raw_file_{create_file(settings.fix.fix_log_path)} {
  if (static_cast<bool>(raw_file_)) {
    auto fd = (*raw_file_.get()).fd();
    file_ = ::fdopen(fd, "w");
  }
}

FIXLog::~FIXLog() {
  try {
    if (file_) {
      ::fflush(file_);
      ::fclose(file_);
    }
  } catch (...) {
  }
}

}  // namespace fix_bridge
}  // namespace roq
