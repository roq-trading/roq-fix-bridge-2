/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <memory>

#include "roq/logging.hpp"

#include "roq/utils/debug/fix/message.hpp"

#include "roq/io/fs/file.hpp"

#include "roq/fix_bridge/settings.hpp"

namespace roq {
namespace fix_bridge {

struct FIXLog final {
  explicit FIXLog(Settings const &);

  ~FIXLog();

  FIXLog(FIXLog const &) = delete;

  template <std::size_t level = 0>
  void sending(uint64_t session_id, std::span<std::byte const> const &message) {
    using namespace std::literals;
    log<level>("OUT"sv, session_id, message);
  }

  template <std::size_t level = 0>
  void received(uint64_t session_id, std::span<std::byte const> const &message) {
    using namespace std::literals;
    log<level>("IN"sv, session_id, message);
  }

 protected:
  template <std::size_t level = 0>
  void log(std::string_view const &direction, uint64_t session_id, std::span<std::byte const> const &message) {
    using namespace std::literals;
    if (settings_.fix.fix_debug) [[unlikely]] {
      log::info<level>("{}[session_id={}]: {}"sv, direction, session_id, utils::debug::fix::Message{message});
    }
    if (file_ != nullptr) [[unlikely]] {
      fmt::print(file_, "{}[session_id={}]: {}\n"sv, direction, session_id, utils::debug::fix::Message{message});
    }
  }

 private:
  Settings const &settings_;
  std::unique_ptr<io::fs::File> raw_file_;
  FILE *file_ = nullptr;
};

}  // namespace fix_bridge
}  // namespace roq
