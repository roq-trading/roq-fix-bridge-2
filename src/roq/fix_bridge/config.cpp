/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/fix_bridge/config.hpp"

#include <toml++/toml.h>

#include <utility>

#include <magic_enum/magic_enum.hpp>

#include "roq/logging.hpp"

#include "roq/utils/compare.hpp"
#include "roq/utils/enum.hpp"

using namespace std::literals;

namespace roq {
namespace fix_bridge {

// === HELPERS ===

namespace {
auto get_order_cancel_policy(auto &settings) {
  if (settings.oms.cancel_on_disconnect) {
    return OrderCancelPolicy::BY_ACCOUNT;
  }
  return OrderCancelPolicy::UNDEFINED;
}

auto create_table(auto &settings) {
  auto path = settings.common.config_file;
  log::info(R"(Parse config_file="{}")"sv, path);
  auto result = toml::parse_file(path);
  return result;
}

void check_empty(auto &node) {
  if (!node.is_table()) {
    return;
  }
  auto &table = *node.as_table();
  auto error = false;
  for (auto &[key, value] : table) {
    roq::log::warn(R"(key="{}")"sv, static_cast<std::string_view>(key));
    error = true;
  }
  if (error) {
    roq::log::fatal("Unexpected"sv);
  }
}

template <typename Callback>
bool find_and_remove(auto &node, std::string_view const &key, Callback callback) {
  if (!node.is_table()) {
    log::warn("Unexpected: node is not a table"sv);
    return false;
  }
  auto &table = *node.as_table();
  auto iter = table.find(key);
  if (iter == table.end()) {
    return false;
  }
  callback((*iter).second);
  table.erase(iter);
  return true;
}

template <typename R>
R get(auto &node, std::string_view const &key) {
  using result_type = std::remove_cvref_t<R>;
  if (!node.is_table()) {
    throw RuntimeError{"Unexpected: node is not a table"sv};
  }
  auto &table = *node.as_table();
  auto iter = table.find(key);
  if (iter == table.end()) {
    throw RuntimeError{R"(Unexpected: did not find key="{}")"sv, key};
  }
  return (*iter).second.template value<result_type>().value();
}

template <typename R>
R get_and_remove(auto &node, std::string_view const &key) {
  using result_type = std::remove_cvref_t<R>;
  if (!node.is_table()) {
    throw RuntimeError{"Unexpected: node is not a table"sv};
  }
  auto &table = *node.as_table();
  auto iter = table.find(key);
  if (iter == table.end()) {
    throw RuntimeError{R"(Unexpected: did not find key="{}")"sv, key};
  }
  result_type result = (*iter).second.template value<result_type>().value();
  table.erase(iter);
  return result;
}

template <typename R>
R maybe_get_and_remove(auto &node, std::string_view const &key) {
  using result_type = std::remove_cvref_t<R>;
  if (!node.is_table()) {
    throw RuntimeError{"Unexpected: node is not a table"sv};
  }
  auto &table = *node.as_table();
  auto iter = table.find(key);
  if (iter == table.end()) {
    return {};
  }
  result_type result = (*iter).second.template value<result_type>().value();
  table.erase(iter);
  return result;
}

template <typename R>
R parse_symbols(auto &table) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  auto iter = table.find("symbols"sv);
  if (iter == std::end(table)) {
    log::fatal("No symbols"sv);
  } else {
    auto &item = (*iter).second;
    if (item.is_table()) {
      auto &table_2 = *item.as_table();
      for (auto &[gateway, node] : table_2) {
        if (node.is_table()) {
          auto &table = *node.as_table();
          auto iter = table.find("regex"sv);
          if (iter == table.end()) {
            log::fatal("expected symbols.{} to contain regex"sv, static_cast<std::string_view>(gateway));
          } else {
            if ((*iter).second.is_string()) {
              auto regex = (*iter).second.template value<std::string_view>();
              result[gateway].emplace(*regex);
            } else if ((*iter).second.is_array()) {
              auto regex_list = (*iter).second.as_array();
              for (auto &iter_2 : *regex_list) {
                if (iter_2.is_string()) {
                  auto regex = iter_2.template value<std::string_view>();
                  result[gateway].emplace(*regex);
                } else {
                  log::fatal("expected string"sv);
                }
              }
            } else {
              log::fatal("expected string or array"sv);
            }
          }
        } else if (gateway == "regex"sv) {
          if (node.is_string()) {
            auto regex = node.template value<std::string_view>();
            result[""sv].emplace(*regex);
          } else if (node.is_array()) {
            auto regex_list = node.as_array();
            for (auto &iter_2 : *regex_list) {
              if (iter_2.is_string()) {
                auto regex = iter_2.template value<std::string_view>();
                result[""sv].emplace(*regex);
              } else {
                log::fatal("expected string"sv);
              }
            }
          } else {
            log::fatal("expected symbols.regex to be string or list"sv);
          }
        } else {
          log::fatal("expected symbols.{} to be a table"sv, static_cast<std::string_view>(gateway));
        }
      }
    } else {
      log::fatal("expected symbols to be a table"sv);
    }
    table.erase(iter);
  }
  return result;
}

template <typename R>
R parse_symbols_2(auto &settings, auto &table, auto const &key) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  auto parse_helper = [&](auto &node) {
    if (!settings.oms.oms_route_by_strategy) {
      log::fatal("not allowed: users.symbols"sv);
    }
    if (node.is_string()) {
      auto tmp = *node.template value<std::string>();
      result.emplace_back(tmp);
    } else if (node.is_array()) {
      auto regex_list = node.as_array();
      for (auto &iter_2 : *regex_list) {
        if (iter_2.is_string()) {
          auto tmp = *iter_2.template value<std::string>();
          result.emplace_back(tmp);
        } else {
          log::fatal("expected string"sv);
        }
      }
    } else {
      if (settings.oms.oms_route_by_strategy) {
        log::fatal("expected users.symbols to be string or list"sv);
      }
    }
  };
  if (find_and_remove(table, key, parse_helper)) {
  } else {
  }
  return result;
}

template <typename R>
R parse_symbols_3(auto &table, auto const &key) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  auto parse_helper = [&](auto &node) {
    if (node.is_string()) {
      auto value = *node.template value<std::string>();
      result.emplace_back(value);
    } else if (node.is_array()) {
      auto regex_list = node.as_array();
      for (auto &iter_2 : *regex_list) {
        if (iter_2.is_string()) {
          auto value = *iter_2.template value<std::string>();
          result.emplace_back(value);
        } else {
          log::fatal("expected string"sv);
        }
      }
    } else {
      log::fatal("expected broadcast.{}.targets_regex to be string or list"sv, key);
    }
  };
  if (find_and_remove(table, key, parse_helper)) {
  } else {
  }
  return result;
}

template <typename R>
R parse_users(auto &settings, auto &table) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  auto parse_helper = [&](auto &node) {
    auto table = node.as_table();
    for (auto &[key, node] : *table) {
      auto &table = *node.as_table();
      auto user_id = static_cast<uint16_t>(std::size(result) + 1);
      auto user = User{
          .user_id = user_id,
          .component = get_and_remove<decltype(User::component)>(table, "component"sv),
          .username = get_and_remove<decltype(User::username)>(table, "username"sv),
          .password = maybe_get_and_remove<decltype(User::password)>(table, "password"sv),
          .account = maybe_get_and_remove<decltype(User::account)>(table, "account"sv),
          .symbols_regex = parse_symbols_2<decltype(User::symbols_regex)>(settings, table, "symbols"sv),
      };
      // validate
      auto error = false;
      if (std::empty(user.username)) {
        log::error("'users.{}.username' is required"sv, static_cast<std::string_view>(key));
        error = true;
      }
      if (settings.oms.oms_route_by_strategy && !std::empty(user.account)) {
        log::error("'users.{}.account' is not allowed when routing by strategy"sv, static_cast<std::string_view>(key));
        error = true;
      }
      if (error) {
        log::fatal("Unexpected: 'users.{}' is incomplete"sv, static_cast<std::string_view>(key));
      }
      check_empty(table);
      // success
      result.emplace(key, std::move(user));
    }
  };
  if (find_and_remove(table, "users"sv, parse_helper)) {
  } else {
    log::fatal(R"(Unexpected: did not find the "users" table)"sv);
  }
  return result;
}

template <typename R>
R parse_statistics(auto &default_values, auto &settings, auto &table) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  auto parse_helper = [&](auto &node) {
    auto table = node.as_table();
    for (auto &[key, node] : *table) {
      auto tmp = magic_enum::enum_cast<StatisticsType>(key);
      if (!tmp.has_value()) {
        log::fatal(R"(Can't map "{}" to StatisticsType)"sv, static_cast<std::string_view>(key));
      }
      auto type = tmp.value();
      auto &table = *node.as_table();
      auto value = get<std::string_view>(table, "fix_md_entry_type"sv);
      auto tmp2 = magic_enum::enum_cast<fix::MDEntryType>(value);
      if (tmp2.has_value()) {
        auto md_entry_type = tmp2.value();
        if (md_entry_type == fix::MDEntryType::UNDEFINED) {
          log::fatal(R"(Can't map "{}" to MDEntryType)"sv, value);
        }
        // allowed?
        switch (md_entry_type) {
          using enum fix::MDEntryType;
          case BID:
          case OFFER:
          case TRADE:
            log::fatal("You're not allowed to remap {}"sv, md_entry_type);
            break;
          default:
            break;
        }
        [[maybe_unused]] auto res = result.emplace(type, md_entry_type);
        assert(res.second);
      } else {
        log::fatal(R"(Unexpected: can't parse "{}" as MDEntryType)"sv, value);
      }
      auto parse_default_value = [&](auto &node) {
        if (settings.messaging.init_missing_md_entry_type_to_zero) {
          auto value = *node.template value<double>();
          default_values.emplace(type, value);
        } else {
          log::fatal("You're not allowed to specify default values without the --init_missing_md_entry_type_to_zero flag"sv);
        }
      };
      find_and_remove(table, "default_value"sv, parse_default_value);
      // validate
      if (std::size(table) != 1) {
        log::error(R"(key="{}" has unexpected fields:)"sv, static_cast<std::string_view>(key));
        for (auto &[k, _] : table) {
          if (k != "fix_md_entry_type"sv) {
            log::warn("  {}"sv, static_cast<std::string_view>(k));
          }
        }
        log::fatal("Unexpected"sv);
      }
    }
  };
  find_and_remove(table, "statistics"sv, parse_helper);
  return result;
}

template <typename R>
R parse_broadcast(auto &table) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  auto parse_helper = [&](auto &node) {
    auto table = node.as_table();
    for (auto &[key, node] : *table) {
      auto broadcast = Broadcast{
          .exchange = get_and_remove<decltype(Broadcast::exchange)>(node, "exchange"sv),
          .source_regex = get_and_remove<decltype(Broadcast::source_regex)>(node, "source_regex"sv),
          .targets_regex = parse_symbols_3<decltype(Broadcast::targets_regex)>(node, "targets_regex"sv),
      };
      // validate
      auto error = false;
      if (std::empty(broadcast.exchange)) {
        log::error("'broadcast.{}.exchange' is required"sv, static_cast<std::string_view>(key));
        error = true;
      }
      if (std::empty(broadcast.source_regex)) {
        log::error("'broadcast.{}.source_regex' is required"sv, static_cast<std::string_view>(key));
        error = true;
      }
      if (std::empty(broadcast.targets_regex)) {
        log::error("'broadcast.{}.targets_regex' is required"sv, static_cast<std::string_view>(key));
        error = true;
      }
      if (error) {
        log::fatal("Unexpected: 'broadcast.{}' is incomplete"sv, static_cast<std::string_view>(key));
      }
      check_empty(node);
      // success
      result.emplace(key, std::move(broadcast));
    }
  };
  find_and_remove(table, "broadcast"sv, parse_helper);
  return result;
}

template <typename R>
R parse_cxl_rej_reason(auto &table) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  auto parse_helper = [&](auto &node) {
    auto table = node.as_table();
    for (auto &[key, node] : *table) {
      auto error = utils::parse_enum<Error>(key);
      auto &table_2 = *node.as_table();
      for (auto &[key_2, node_2] : table_2) {
        auto value = node_2.template value<uint32_t>().value();
        if (value < 100) {
          log::fatal("Unexpected: custom CxlRejReason values must be at least 100 (got {})"sv, value);
        }
        if (key_2 == "cxl_rej_reason"sv) {
          result[error] = static_cast<fix::CxlRejReason>(value);
        } else {
          log::fatal("Unexpected: {}"sv, key_2);
        }
      }
    }
  };
  find_and_remove(table, "errors"sv, parse_helper);
  return result;
}

void check_unique_usernames(auto &users) {
  utils::unordered_set<std::string> usernames;
  for (auto &[_, user] : users) {
    auto &username = user.username;
    assert(!std::empty(username));
    auto res = usernames.emplace(username);
    if (!res.second) {
      log::fatal(R"(Unexpected: duplicated username="{}")"sv, username);
    }
  }
}

void check_unique_accounts(auto &users) {
  utils::unordered_set<std::string> accounts;
  for (auto &[_, user] : users) {
    auto &account = user.account;
    if (!std::empty(account)) {
      auto res = accounts.emplace(account);
      if (!res.second) {
        log::fatal(R"(Unexpected: duplicated account="{}")"sv, account);
      }
    }
  }
}

}  // namespace

// === IMPLEMENTATION ===

Config::Config(Settings const &settings) : Config{settings, create_table(settings)} {
}

Config::Config(Settings const &settings, auto &&table)
    : order_cancel_policy_{get_order_cancel_policy(settings)}, symbols{parse_symbols<decltype(symbols)>(table)},
      users{parse_users<decltype(users)>(settings, table)}, statistics{parse_statistics<decltype(statistics)>(default_values, settings, table)},
      broadcast{parse_broadcast<decltype(broadcast)>(table)}, cxl_rej_reason{parse_cxl_rej_reason<decltype(cxl_rej_reason)>(table)} {
  check_empty(table);
  log::info<1>("config={}"sv, *this);
  log::debug("config={}"sv, *this);
  check_unique_usernames(users);
  if (!settings.oms.oms_route_by_strategy) {
    check_unique_accounts(users);
  }
}

void Config::dispatch(Handler &handler) const {
  // settings
  auto settings = client::Settings{
      .order_cancel_policy = order_cancel_policy_,
      .order_management = OrderManagement::FIX,
  };
  handler(settings);
  // symbols
  for (auto &[gateway, subscriptions] : symbols) {
    for (auto &regex : subscriptions) {
      auto symbol = client::Symbol{
          .regex = regex,
          .exchange = gateway,
      };
      handler(symbol);
    }
  }
  // accounts
  for (auto &[_, user] : users) {
    if (!std::empty(user.account)) {
      auto account = client::Account{
          .regex = user.account,
      };
      handler(account);
    }
  }
}

}  // namespace fix_bridge
}  // namespace roq
