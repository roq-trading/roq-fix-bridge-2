/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <string>
#include <vector>

namespace roq {
namespace fix_bridge {

struct User final {
  uint16_t user_id = {};
  std::string component;
  std::string username;
  std::string password;
  uint32_t strategy_id = {};
  std::string account;
  std::vector<std::string> accounts_regex;
  std::vector<std::string> symbols_regex;
};

}  // namespace fix_bridge
}  // namespace roq

// user

template <>
struct fmt::formatter<roq::fix_bridge::User> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::fix_bridge::User const &value, format_context &context) const {
    using namespace std::literals;
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(user_id={}, )"
        R"(component="{}", )"
        R"(username="{}", )"
        R"(password=***, )"
        R"(strategy_id={}, )"
        R"(account="{}", )"
        R"(symbols_regex=[{}])"
        R"(}})"sv,
        value.user_id,
        value.component,
        value.username,
        value.strategy_id,
        value.account,
        fmt::join(value.accounts_regex, ", "sv),
        fmt::join(value.symbols_regex, ", "sv));
  }
};
