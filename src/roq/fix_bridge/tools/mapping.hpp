/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <span>
#include <string>
#include <string_view>

#include "roq/utils/container.hpp"

#include "roq/utils/regex/pattern.hpp"

namespace roq {
namespace fix_bridge {
namespace tools {

struct Mapping final {
  Mapping(std::string_view const &name, std::string_view const &exchange, std::string_view const &source_regex, std::string_view const &targets_regex);

  Mapping(
      std::string_view const &name, std::string_view const &exchange, std::string_view const &source_regex, std::span<std::string const> const &targets_regex);

  Mapping(Mapping const &) = delete;
  Mapping(Mapping &&) = default;

  bool add(std::string_view const &exchange, std::string_view const &symbol);

  template <typename Callback>
  bool dispatch([[maybe_unused]] std::string_view const &exchange, std::string_view const &symbol, Callback &&callback) const {
    auto iter = outbound_.find(symbol);
    if (iter == std::end(outbound_)) {
      return false;
    }
    auto &symbols = (*iter).second;
    for (auto &symbol_ : symbols) {
      callback(exchange_, symbol_);
    }
    return true;
  }

 protected:
  void add_source(std::string_view const &key, std::string_view const &symbol);
  void add_target(std::string_view const &key, std::string_view const &symbol);

 private:
  std::string const name_;
  std::string const exchange_;
  utils::regex::Pattern source_regex_;
  std::vector<utils::regex::Pattern> targets_regex_;

  utils::unordered_set<std::string> processed_;  // debug: must only be called once
  utils::unordered_map<std::string, std::string> source_;
  utils::unordered_map<std::string, utils::unordered_set<std::string>> target_;
  utils::unordered_map<std::string, utils::unordered_set<std::string>> outbound_;
};

}  // namespace tools
}  // namespace fix_bridge
}  // namespace roq
