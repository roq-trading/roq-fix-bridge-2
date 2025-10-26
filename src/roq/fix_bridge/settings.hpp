/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/client.hpp"

#include "roq/fix_bridge/flags/common.hpp"
#include "roq/fix_bridge/flags/fix.hpp"
#include "roq/fix_bridge/flags/messaging.hpp"
#include "roq/fix_bridge/flags/oms.hpp"
#include "roq/fix_bridge/flags/test.hpp"

namespace roq {
namespace fix_bridge {

struct Settings final : public client::flags::Settings {
  explicit Settings(args::Parser const &);

  flags::Common common;
  flags::FIX fix;
  flags::Messaging messaging;
  flags::OMS oms;
  flags::Test test;
};

}  // namespace fix_bridge
}  // namespace roq

template <>
struct fmt::formatter<roq::fix_bridge::Settings> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::fix_bridge::Settings const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(common={}, )"
        R"(fix={}, )"
        R"(messaging={}, )"
        R"(oms={}, )"
        R"(test={}, )"
        R"(client={})"
        R"(}})"sv,
        value.common,
        value.fix,
        value.messaging,
        value.oms,
        value.test,
        static_cast<roq::client::Settings2 const &>(value));
  }
};
