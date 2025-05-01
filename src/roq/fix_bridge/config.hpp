/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <string>
#include <vector>

#include "roq/api.hpp"

#include "roq/client.hpp"

#include "roq/utils/container.hpp"

#include "roq/fix/common.hpp"

#include "roq/fix_bridge/settings.hpp"

namespace roq {
namespace fix_bridge {

struct User final {
  uint16_t user_id = {};
  std::string component;
  std::string username;
  std::string password;
  std::string account;
  std::vector<std::string> symbols_regex;
};

struct Broadcast final {
  std::string exchange;
  std::string source_regex;
  std::vector<std::string> targets_regex;
};

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

// symbols

template <>
struct fmt::formatter<std::pair<std::string const, roq::utils::unordered_set<std::string>>> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(std::pair<std::string const, roq::utils::unordered_set<std::string>> const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(context.out(), "{}={{{}}}"sv, value.first, fmt::join(value.second, ", "sv));
  }
};

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
        R"(account="{}", )"
        R"(symbols_regex=[{}])"
        R"(}})"sv,
        value.user_id,
        value.component,
        value.username,
        value.account,
        fmt::join(value.symbols_regex, ", "sv));
  }
};

template <>
struct fmt::formatter<std::pair<std::string const, roq::fix_bridge::User>> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(std::pair<std::string const, roq::fix_bridge::User> const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(context.out(), "{}={}"sv, value.first, value.second);
  }
};

// default values

template <>
struct fmt::formatter<std::pair<roq::StatisticsType const, double>> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(std::pair<roq::StatisticsType const, double> const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(context.out(), "{}={}"sv, value.first, value.second);
  }
};

// statistics

template <>
struct fmt::formatter<std::pair<roq::StatisticsType const, roq::fix::MDEntryType>> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(std::pair<roq::StatisticsType const, roq::fix::MDEntryType> const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(context.out(), "{}={}({})"sv, value.first, value.second, roq::fix::Codec<roq::fix::MDEntryType>::encode(value.second));
  }
};

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

template <>
struct fmt::formatter<std::pair<std::string const, roq::fix_bridge::Broadcast>> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(std::pair<std::string const, roq::fix_bridge::Broadcast> const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(context.out(), "{}={}"sv, value.first, value.second);
  }
};

// config

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
