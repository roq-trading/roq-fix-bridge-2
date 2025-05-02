/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <string>

#include "roq/api.hpp"

#include "roq/client.hpp"

#include "roq/utils/container.hpp"

#include "roq/fix/common.hpp"

#include "roq/fix_bridge/broadcast.hpp"
#include "roq/fix_bridge/settings.hpp"
#include "roq/fix_bridge/user.hpp"

namespace roq {
namespace fix_bridge {

struct Config final : public client::Config {
  explicit Config(Settings const &);

  Config(Config const &) = delete;

 protected:
  void dispatch(Handler &) const override;

 private:
  Config(Settings const &, auto &&table);

  OrderCancelPolicy const order_cancel_policy_;

 public:
  utils::unordered_map<std::string, utils::unordered_set<std::string>> const symbols;
  utils::unordered_map<std::string, User> const users;
  utils::unordered_map<StatisticsType, double> default_values;  // XXX make const
  utils::unordered_map<StatisticsType, fix::MDEntryType> const statistics;
  utils::unordered_map<std::string, Broadcast> const broadcast;
  utils::unordered_map<Error, fix::CxlRejReason> const cxl_rej_reason;
};

}  // namespace fix_bridge
}  // namespace roq

template <>
struct fmt::formatter<roq::fix_bridge::Config> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::fix_bridge::Config const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        "{{"
        "symbols={{{}}}, "
        "users={{{}}}, "
        "default_values={{{}}}, "
        "statistics={{{}}}, "
        "broadcast={{{}}}, "
        "cxl_rej_reason={{{}}}"
        "}}"sv,
        fmt::join(value.symbols, ", "sv),
        fmt::join(value.users, ", "sv),
        fmt::join(value.default_values, ", "sv),
        fmt::join(value.statistics, ", "sv),
        fmt::join(value.broadcast, ", "sv),
        fmt::join(value.cxl_rej_reason, ", "sv));
  }
};
