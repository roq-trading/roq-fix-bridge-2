/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <set>

#include <magic_enum/magic_enum.hpp>

#include "roq/utils/container.hpp"

#include "roq/utils/metrics/profile.hpp"

#include "roq/fix/bridge/manager.hpp"

#include "roq/fix_bridge/config.hpp"
#include "roq/fix_bridge/fix_log.hpp"
#include "roq/fix_bridge/session.hpp"

#include "roq/fix_bridge/tools/crypto.hpp"
#include "roq/fix_bridge/tools/mapping.hpp"

namespace roq {
namespace fix_bridge {

struct Shared final {
  Shared(client::Dispatcher &, fix::bridge::Manager &, Settings const &, Config const &, size_t source_count);

  Shared(Shared &&) = delete;
  Shared(Shared const &) = delete;

  // session

  template <typename Callback>
  void session_logon(
      uint64_t session_id,
      std::string_view const &component,
      std::string_view const &username,
      std::string_view const &password,
      std::string_view const &raw_data,
      Callback callback) {
    auto [account, user, reason] = session_logon_helper(session_id, component, username, password, raw_data);
    auto success = std::empty(reason);
    auto user_id = user ? (*user).user_id : 0;
    callback(success, account, user_id, reason);
  }

  void session_logout(uint64_t session_id);

  // sources

  void add_source(uint8_t source_id, std::string_view const &source_name);
  void remove_source(uint8_t source_id, std::string_view const &source_name);

  template <typename Callback>
  bool get_sources(Callback callback) {
    if (std::empty(gateways_)) {
      return false;
    }
    for (auto const &[source_name, tmp] : gateways_) {
      callback(tmp.first, source_name, tmp.second);
    }
    return true;
  }

  void operator()(Event<Disconnected> const &);
  void operator()(Event<StreamStatus> const &);

  void operator()(Event<GatewaySettings> const &);

  // broadcast

  void update_broadcast_table(std::string_view const &exchange, std::string_view const &symbol);

  template <typename Callback>
  bool broadcast(std::string_view const &exchange, std::string_view const &symbol, Callback callback) {
    auto result = false;
    for (auto &mapping : broadcast_mappings_) {
      mapping.dispatch(exchange, symbol, [&](auto &exchange, auto &symbol) {
        get_market(exchange, symbol, [&](auto &market) {
          result = true;
          callback(market);
        });
      });
    }
    return result;
  }

  // routing v2

  bool add_route(uint64_t session_id, uint32_t strategy_id);
  bool remove_route(uint64_t session_id, uint32_t strategy_id);
  void remove_all_routes(uint64_t session_id);

  // metrics

  void operator()(metrics::Writer &) const;

 protected:
  std::tuple<std::string_view, User const *, std::string> session_logon_helper(
      uint64_t session_id,
      std::string_view const &component,
      std::string_view const &username,
      std::string_view const &password,
      std::string_view const &raw_data);

 public:
  Settings const &settings;
  client::Dispatcher &dispatcher;
  fix::bridge::Manager &bridge;

  FIXLog fix_log;

  struct {
    utils::metrics::Profile parse,                                                  //
        logon, logout, test_request, resend_request, reject, heartbeat,             //
        trading_session_status_request,                                             //
        security_list_request,                                                      //
        security_definition_request, security_status_request, market_data_request,  //
        user_request, order_status_request, order_mass_status_request,              //
        new_order_single, order_cancel_request, order_cancel_replace_request,
        order_mass_cancel_request,     //
        trade_capture_report_request,  //
        request_for_positions,         //
        mass_quote, quote_cancel;
  } profile;

  struct {
    utils::metrics::internal_latency_t events, end_to_end;
    utils::metrics::external_latency_t round_trip_gateway, round_trip_broker, round_trip_exchange;
  } latency;

 private:
  tools::Crypto crypto_;
  // sessions
  // - config
  utils::unordered_map<std::string, User> const username_to_user_;
  // - username
  utils::unordered_map<uint64_t, std::pair<std::string, std::string>> session_to_username_and_account_;
  utils::unordered_map<std::string, uint64_t> username_to_session_;
  utils::unordered_set<std::string> locked_usernames_;
  utils::unordered_map<std::string, utils::unordered_set<uint64_t>> account_to_sessions_;
  // gateways
  utils::unordered_map<std::string, std::pair<uint8_t, bool>> gateways_;
  std::vector<utils::unordered_map<std::string, ConnectionStatus>> oms_status_;
  std::vector<Mask<Filter>> oms_cancel_all_orders_;
  // markets
  // - mapping
  std::vector<tools::Mapping> broadcast_mappings_;
  // route v2
  utils::unordered_map<uint32_t, uint64_t> strategy_id_to_session_id_;
  utils::unordered_map<uint64_t, utils::unordered_set<uint32_t>> session_id_to_strategy_id_;
};

}  // namespace fix_bridge
}  // namespace roq
