/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <string>
#include <vector>

namespace roq {
namespace fix_bridge {

struct Broadcast final {
  std::string exchange;
  std::string source_regex;
  std::vector<std::string> targets_regex;
};

}  // namespace fix_bridge
}  // namespace roq

// broadcast

template <>
struct fmt::formatter<roq::fix_bridge::Broadcast> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::fix_bridge::Broadcast const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(exchange="{}", )"
        R"(source_regex="{}", )"
        R"(targets_regex=[{}])"
        R"(}})"sv,
        value.exchange,
        value.source_regex,
        fmt::join(value.targets_regex, ", "sv));
  }
};
