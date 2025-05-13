/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/fix_bridge/shared.hpp"

#include <algorithm>
#include <utility>

#include "roq/logging.hpp"

#include "roq/utils/update.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/fix/map.hpp"

using namespace std::literals;

namespace roq {
namespace fix_bridge {

// === HELPERS ===

namespace {
auto extract_username_to_user(auto &settings, auto &config) {
  utils::unordered_map<std::string, User> result;
  utils::unordered_map<std::string, std::string> account_to_username;
  for (auto &[_, user] : config.users) {
    auto res = result.emplace(user.username, user);
    if (!res.second) {
      log::fatal(R"(Unexpected: username="{}" configured more than once)"sv, user.username);
    }
    // validate account
    if (settings.oms.oms_route_by_strategy) {
      // special case: validation must be at run-time
    } else {
      if (!std::empty(user.account)) {
        auto res = account_to_username.emplace(user.account, user.username);
        if (!res.second) {
          log::fatal(
              R"(Unexpected: account="{}" configured more than once (username="{}" and username="{}"))"sv, user.account, user.username, (*res.first).second);
        }
      }
    }
  }
  return result;
}

auto extract_broadcast_mappings(auto &config) {
  std::vector<tools::Mapping> result;
  for (auto &[name, broadcast] : config.broadcast) {
    result.emplace_back(name, broadcast.exchange, broadcast.source_regex, broadcast.targets_regex);
  }
  return result;
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto const &function) : utils::metrics::Factory{settings.app.name, "session"sv, function} {}
};

auto create_internal_latency(auto &settings) {
  auto labels = fmt::format(R"(source="{}")"sv, settings.app.name);
  return utils::metrics::internal_latency_t{labels};
}

auto create_external_latency(auto &settings, auto const &function) {
  auto labels = fmt::format(R"(source="{}", function="{}")"sv, settings.app.name, function);
  return utils::metrics::external_latency_t{labels};
}
}  // namespace

// === IMPLEMENTATION ===

Shared::Shared(client::Dispatcher &dispatcher, fix::bridge::Manager &bridge, Settings const &settings, Config const &config, size_t source_count)
    : settings{settings}, dispatcher{dispatcher}, bridge{bridge}, fix_log{settings},
      profile{
          .parse = create_metrics(settings, "parse"sv),
          .logon = create_metrics(settings, "logon"sv),
          .logout = create_metrics(settings, "logout"sv),
          .test_request = create_metrics(settings, "test_request"sv),
          .resend_request = create_metrics(settings, "resend_request"sv),
          .reject = create_metrics(settings, "reject"sv),
          .heartbeat = create_metrics(settings, "heartbeat"sv),
          .trading_session_status_request = create_metrics(settings, "trading_session_status_request"sv),
          .security_list_request = create_metrics(settings, "security_list_request"sv),
          .security_definition_request = create_metrics(settings, "security_definition_request"sv),
          .security_status_request = create_metrics(settings, "security_status_request"sv),
          .market_data_request = create_metrics(settings, "market_data_request"sv),
          .user_request = create_metrics(settings, "user_request"sv),
          .order_status_request = create_metrics(settings, "order_status_request"sv),
          .order_mass_status_request = create_metrics(settings, "order_mass_status_request"sv),
          .new_order_single = create_metrics(settings, "new_order_single"sv),
          .order_cancel_request = create_metrics(settings, "order_cancel_request"sv),
          .order_cancel_replace_request = create_metrics(settings, "order_cancel_replace_request"sv),
          .order_mass_cancel_request = create_metrics(settings, "order_mass_cancel_request"sv),
          .trade_capture_report_request = create_metrics(settings, "trade_capture_report_request"sv),
          .request_for_positions = create_metrics(settings, "request_for_positions"sv),
          .mass_quote = create_metrics(settings, "mass_quote"sv),
          .quote_cancel = create_metrics(settings, "quote_cancel"sv),
      },
      latency{
          .events = create_internal_latency(settings),
          .end_to_end = create_internal_latency(settings),
          .round_trip_gateway = create_external_latency(settings, "gateway"sv),
          .round_trip_broker = create_external_latency(settings, "broker"sv),
          .round_trip_exchange = create_external_latency(settings, "exchange"sv),
      },
      crypto_{settings.fix.fix_auth_method, settings.fix.fix_auth_timestamp_tolerance}, username_to_user_{extract_username_to_user(settings, config)},
      oms_status_(source_count), oms_cancel_all_orders_(source_count), broadcast_mappings_(extract_broadcast_mappings(config)) {
}

// session

std::tuple<std::string_view, User const *, std::string> Shared::session_logon_helper(
    uint64_t session_id,
    std::string_view const &component,
    std::string_view const &username,
    std::string_view const &password,
    std::string_view const &raw_data) {
  log::info(R"(Session: ADD session_id={} (component="{}", username="{}"))"sv, session_id, component, username);
  // XXX TODO validate component
  auto iter = username_to_user_.find(username);
  if (iter == std::end(username_to_user_)) {
    return {{}, {}, fmt::format(R"(Unknown username="{}")"sv, username)};
  }
  auto &user = (*iter).second;
  if (std::empty(user.account)) {
    return {};
  }
  if (settings.test.block_client_on_not_ready) {
    auto ready = true;
    for (auto &item : oms_status_) {
      auto iter = item.find(user.account);
      if (iter != item.end() && (*iter).second == ConnectionStatus::READY) {
      } else {
        ready = false;
      }
    }
    if (!ready) {
      return {{}, {}, fmt::format(R"(Not ready: account="{}")"sv, user.account)};
    }
  }
  if (component != user.component) {
    return {{}, {}, "Invalid component"s};
  }
  if (!crypto_.validate(password, user.password, raw_data)) {
    return {{}, {}, "Invalid password"s};
  }
  if (!locked_usernames_.emplace(username).second) {
    return {user.account, &user, fmt::format(R"(Username "{}" already connected)"sv, username)};
  }
  auto &sessions = account_to_sessions_[user.account];
  if (!settings.oms.oms_route_by_strategy && !std::empty(sessions)) {
    return {user.account, {}, fmt::format(R"(Account "{}" already connected)"sv, user.account)};
  }
  sessions.emplace(session_id);
  session_to_username_and_account_.try_emplace(session_id, username, user.account);
  assert(username_to_session_.find(username) == std::end(username_to_session_));
  username_to_session_.emplace(username, session_id);
  return {user.account, &user, {}};
}

void Shared::session_logout(uint64_t session_id) {
  log::info("Session: REMOVE session_id={}"sv, session_id);
  auto iter = session_to_username_and_account_.find(session_id);
  if (iter != std::end(session_to_username_and_account_)) {
    auto &[username, account] = (*iter).second;
    [[maybe_unused]] auto count = locked_usernames_.erase(username);
    assert(count != 0);
    log::info(R"(User username="{}" has been RELEASED)"sv, username);
    assert(username_to_session_.find(username) != std::end(username_to_session_));
    username_to_session_.erase(username);
    auto &sessions = account_to_sessions_[account];
    sessions.erase(session_id);
    session_to_username_and_account_.erase(iter);
  }
}

// sources

void Shared::add_source(uint8_t source_id, std::string_view const &source_name) {
  auto &tmp = gateways_[source_name];
  if (tmp.second) {
    log::warn(R"(Unexpected: gateway already connected (source_id={}, source_name="{}")"sv, source_id, source_name);
  } else {
    tmp = {source_id, true};
  }
}

void Shared::remove_source(uint8_t source_id, std::string_view const &source_name) {
  auto &tmp = gateways_[source_name];
  if (tmp.second) {
    tmp = {source_id, false};
  } else {
    log::warn(R"(Unexpected: gateway not connected (source_id={}, source_name="{}")"sv, source_id, source_name);
  }
}

void Shared::operator()(Event<Disconnected> const &event) {
  auto &[message_info, disconnected] = event;
  oms_status_[message_info.source].clear();
  if (settings.test.block_client_on_not_ready) {
    log::warn("Order management is NOT READY -- all sessions will be logged out!"sv);
    // get_all_sessions([&](auto &session) { session.logout(fix::codec::Error::NOT_READY); });
  }
}

void Shared::operator()(Event<StreamStatus> const &event) {
  auto &[message_info, stream_status] = event;
  if (std::empty(stream_status.account)) {
    return;
  }
  if (!stream_status.supports.has(SupportType::ORDER)) {
    return;
  }
  auto &current = oms_status_[message_info.source][stream_status.account];
  auto previous = std::exchange(current, stream_status.connection_status);
  if (current != previous) {
    if (previous == ConnectionStatus::READY) {
      if (settings.test.block_client_on_not_ready) {
        log::warn(R"(Order management for account="{}" is NOT READY -- all sessions will be logged out!)"sv, stream_status.account);
        // get_all_sessions(stream_status.account, [&](auto &session) { session.logout(fix::codec::Error::NOT_READY); });
      }
    }
  }
}

void Shared::operator()(Event<GatewaySettings> const &event) {
  auto &[message_info, gateway_settings] = event;
  oms_cancel_all_orders_[message_info.source] = gateway_settings.oms_cancel_all_orders;
}

// market

void Shared::update_broadcast_table(std::string_view const &exchange, std::string_view const &symbol) {
  for (auto &mapping : broadcast_mappings_) {
    mapping.add(exchange, symbol);
  }
}

// routing v2

bool Shared::add_route(uint64_t session_id, uint32_t strategy_id) {
  auto iter = strategy_id_to_session_id_.find(strategy_id);
  if (iter == std::end(strategy_id_to_session_id_)) {
    strategy_id_to_session_id_.try_emplace(strategy_id, session_id);
    session_id_to_strategy_id_[session_id].emplace(strategy_id);
    log::info(R"(DEBUG: ROUTE ADD strategy_id={} <==> session_id={})"sv, strategy_id, session_id);
    return true;
  }
  return false;
}

bool Shared::remove_route(uint64_t session_id, uint32_t strategy_id) {
  auto iter = strategy_id_to_session_id_.find(strategy_id);
  if (iter == std::end(strategy_id_to_session_id_) || (*iter).second != session_id) {
    return false;
  }
  log::info(R"(DEBUG ROUTE REMOVE strategy_id={} <==> session_id={})"sv, strategy_id, session_id);
  session_id_to_strategy_id_[session_id].erase(strategy_id);
  strategy_id_to_session_id_.erase(iter);
  return true;
}

void Shared::remove_all_routes(uint64_t session_id) {
  auto iter = session_id_to_strategy_id_.find(session_id);
  if (iter == std::end(session_id_to_strategy_id_)) {
    return;
  }
  for (auto strategy_id : (*iter).second) {
    log::info(R"(DEBUG: ROUTE REMOVE strategy_id={} <==> session_id={})"sv, strategy_id, session_id);
    strategy_id_to_session_id_.erase(strategy_id);
  }
  session_id_to_strategy_id_.erase(iter);
}

/*
bool Shared::is_cancel_all_orders_supported(uint8_t source, Mask<Filter> filter) const {
  auto supported = oms_cancel_all_orders_[source];
  log::warn("DEBUG: supported={}, filter={}"sv, supported, filter);
  return supported.has_all(filter);
}

StatisticsType Shared::to_statistics_type(fix::MDEntryType value) {
  auto index = magic_enum::enum_index(value);
  if (!index.has_value())
    log::fatal("Unexpected: internal error"sv);
  return md_entry_to_statistics[index.value()];
}

fix::MDEntryType Shared::to_md_entry_type(StatisticsType value) {
  auto index = magic_enum::enum_index(value);
  if (!index.has_value())
    log::fatal("Unexpected: internal error"sv);
  return statistics_to_md_entry[index.value()];
}

double Shared::default_value(StatisticsType value) {
  auto iter = default_values.find(value);
  return iter == std::end(default_values) ? 0.0 : (*iter).second;
}
*/

// metrics

void Shared::operator()(metrics::Writer &writer) const {
  writer
      // profile
      .write(profile.parse, metrics::Type::PROFILE)
      .write(profile.logon, metrics::Type::PROFILE)
      .write(profile.logout, metrics::Type::PROFILE)
      .write(profile.test_request, metrics::Type::PROFILE)
      .write(profile.resend_request, metrics::Type::PROFILE)
      .write(profile.reject, metrics::Type::PROFILE)
      .write(profile.heartbeat, metrics::Type::PROFILE)
      .write(profile.trading_session_status_request, metrics::Type::PROFILE)
      .write(profile.security_list_request, metrics::Type::PROFILE)
      .write(profile.security_definition_request, metrics::Type::PROFILE)
      .write(profile.security_status_request, metrics::Type::PROFILE)
      .write(profile.market_data_request, metrics::Type::PROFILE)
      .write(profile.user_request, metrics::Type::PROFILE)
      .write(profile.order_status_request, metrics::Type::PROFILE)
      .write(profile.order_mass_status_request, metrics::Type::PROFILE)
      .write(profile.new_order_single, metrics::Type::PROFILE)
      .write(profile.order_cancel_request, metrics::Type::PROFILE)
      .write(profile.order_cancel_replace_request, metrics::Type::PROFILE)
      .write(profile.order_mass_cancel_request, metrics::Type::PROFILE)
      .write(profile.trade_capture_report_request, metrics::Type::PROFILE)
      .write(profile.request_for_positions, metrics::Type::PROFILE)
      .write(profile.mass_quote, metrics::Type::PROFILE)
      .write(profile.quote_cancel, metrics::Type::PROFILE)
      //
      .write(latency.events, metrics::Type::EVENTS)
      .write(latency.end_to_end, metrics::Type::END_TO_END_LATENCY)
      .write(latency.round_trip_gateway, metrics::Type::REQUEST_LATENCY)
      .write(latency.round_trip_broker, metrics::Type::REQUEST_LATENCY)
      .write(latency.round_trip_exchange, metrics::Type::REQUEST_LATENCY);
}

}  // namespace fix_bridge
}  // namespace roq
