/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/fix_bridge/controller.hpp"

#include "roq/utils/safe_cast.hpp"

using namespace std::literals;

namespace roq {
namespace fix_bridge {

// === HELPERS ===

namespace {
auto create_bridge(auto &handler, auto &settings, auto &config) {
  std::vector<std::pair<StatisticsType, double>> default_values;
  for (auto &item : config.default_values) {
    default_values.emplace_back(item);
  }
  std::vector<std::pair<StatisticsType, fix::MDEntryType>> statistics;
  for (auto &item : config.statistics) {
    statistics.emplace_back(item);
  }
  std::vector<std::pair<Error, fix::CxlRejReason>> cxl_rej_reason_mapping;
  for (auto &item : config.cxl_rej_reason) {
    cxl_rej_reason_mapping.emplace_back(item);
  }
  auto options = fix::bridge::Manager::Options{
      // session
      .comp_id = settings.fix.fix_comp_id,
      .logon_timeout = utils::safe_cast(settings.fix.fix_logon_timeout),
      .logon_heartbeat_min = utils::safe_cast(settings.fix.fix_logon_heartbeat_min),
      .logon_heartbeat_max = utils::safe_cast(settings.fix.fix_logon_heartbeat_max),
      .heartbeat_freq = utils::safe_cast(settings.fix.fix_heartbeat_freq),
      // market data
      .top_of_book_from_market_by_price = settings.messaging.top_of_book_from_market_by_price,
      .default_values = default_values,
      .statistics = statistics,
      // order management
      .route_by_strategy = settings.oms.oms_route_by_strategy,
      .strict_checking = settings.test.strict_checking,
      .disable_req_id_validation = settings.fix.fix_disable_req_id_validation,
      .cxl_rej_reason_mapping = cxl_rej_reason_mapping,
      // test:
      // - market data
      .test_md_entry_position_no = settings.test.test_md_entry_position_no,
      .test_init_missing_md_entry_type_to_zero = settings.messaging.init_missing_md_entry_type_to_zero,
      // - order management
      .test_request_latency = settings.test.test_request_latency,
      .test_terminate_on_timeout = settings.test.terminate_on_timeout,
      .test_silence_non_final_order_ack = settings.test.silence_non_final_order_ack,
      .test_skip_order_download = settings.test.skip_order_download,
      // - other
      .test_terminate_on_disconnect{settings.test.terminate_on_disconnect},
      .test_block_client_on_not_ready{settings.test.block_client_on_not_ready},
      .default_decimal_digits = settings.test.test_default_decimal_digits,
  };
  return fix::bridge::Manager::create(handler, options);
}

template <typename R>
auto create_username_to_user(auto &settings, auto &config) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
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

}  // namespace

// === IMPLEMENTATION ===

Controller::Controller(client::Dispatcher &dispatcher, Settings const &settings, Config const &config, size_t source_count)
    : dispatcher_{dispatcher}, bridge_{create_bridge(*this, settings, config)}, shared_{dispatcher_, *bridge_, settings, config, source_count},
      session_manager_{shared_}, username_to_user_{create_username_to_user<decltype(username_to_user_)>(settings, config)},
      crypto_{settings.fix.fix_auth_method, settings.fix.fix_auth_timestamp_tolerance} {
}

void Controller::operator()(Event<Start> const &event) {
  session_manager_(event);
}

void Controller::operator()(Event<Stop> const &event) {
  session_manager_(event);
}

void Controller::operator()(Event<Timer> const &event) {
  (*bridge_)(event);
  session_manager_(event);
}

void Controller::operator()(Event<Connected> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<Disconnected> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<Control> const &event) {
  auto &[message_info, control] = event;
  log::info("[{}:{}] control"sv, message_info.source, message_info.source_name);
  auto state = [&]() -> State {
    switch (control.action) {
      using enum Action;
      case UNDEFINED:
        return State::UNDEFINED;
      case ENABLE:
        return State::ENABLED;
      case DISABLE:
        return State::DISABLED;
    }
    std::abort();
  }();
  if (control.strategy_id != 0) {
    // XXX FIXME TODO check for legs update
    auto strategy_update = StrategyUpdate{
        .user = {},  // note! client library will set this
        .strategy_id = control.strategy_id,
        .description = {},  // note! client library will set this
        .state = state,
        .update_type = UpdateType::INCREMENTAL,
    };
    dispatcher_(strategy_update);
  } else {
    auto service_update = ServiceUpdate{
        .user = {},               // note! client library will set this
        .description = {},        // note! client library will set this
        .connection_status = {},  // note! client library will set this
        .state = state,
        .update_type = UpdateType::INCREMENTAL,
    };
    dispatcher_(service_update);
  }
}

void Controller::operator()(Event<DownloadBegin> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<DownloadEnd> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<Ready> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<GatewaySettings> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<StreamStatus> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<GatewayStatus> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<ReferenceData> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<MarketStatus> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<TopOfBook> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<MarketByPriceUpdate> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<TradeSummary> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<StatisticsUpdate> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<OrderAck> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<OrderUpdate> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<TradeUpdate> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<FundsUpdate> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(Event<PositionUpdate> const &event) {
  (*bridge_)(event);
}

void Controller::operator()(metrics::Writer &writer) const {
  shared_(writer);
}

// bridge

std::pair<fix::codec::Error, std::string_view> Controller::operator()(fix::bridge::Manager::Credentials const &credentials) {
  auto iter = username_to_user_.find(credentials.username);
  if (iter == std::end(username_to_user_)) {
    log::error(R"(Unknown username "{}")"sv, credentials.username);
    return {fix::codec::Error::INVALID_USERNAME, {}};
  }
  auto &user = (*iter).second;
  if (credentials.component != user.component) {
    log::error(R"(Unexpected component "{}", expected "{}")"sv, credentials.component, user.component);
    return {fix::codec::Error::INVALID_COMPONENT, {}};
  }
  if (!crypto_.validate(credentials.password, user.password, credentials.raw_data)) {
    log::error("Unexpected password");
    return {fix::codec::Error::INVALID_PASSWORD, {}};
  }
  return {{}, user.account};
}

void Controller::operator()(CreateOrder const &create_order, uint8_t source) {
  dispatcher_.send(create_order, source);
}

void Controller::operator()(ModifyOrder const &modify_order, uint8_t source) {
  dispatcher_.send(modify_order, source);
}

void Controller::operator()(CancelOrder const &cancel_order, uint8_t source) {
  dispatcher_.send(cancel_order, source);
}

void Controller::operator()(CancelAllOrders const &cancel_all_orders) {
  dispatcher_.broadcast(cancel_all_orders);
}

void Controller::operator()(roq::MassQuote const &mass_quote, uint8_t source) {
  dispatcher_.send(mass_quote, source);
}

void Controller::operator()(CancelQuotes const &cancel_quotes, uint8_t source) {
  dispatcher_.send(cancel_quotes, source);
}

void Controller::operator()(Trace<fix::bridge::Manager::Disconnect> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::Reject> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::Logon> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::Logout> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::Heartbeat> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::TestRequest> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::BusinessMessageReject> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::UserResponse> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::TradingSessionStatus> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::SecurityList> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::SecurityDefinition> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::SecurityStatus> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::MarketDataRequestReject> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::MarketDataSnapshotFullRefresh> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::MarketDataIncrementalRefresh> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::ExecutionReport> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::OrderCancelReject> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::OrderMassCancelReport> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::TradeCaptureReportRequestAck> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::TradeCaptureReport> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::RequestForPositionsAck> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::PositionReport> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::MassQuoteAck> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::QuoteStatusReport> const &event, uint64_t session_id) {
  dispatch(event, session_id);
}

void Controller::operator()(Trace<fix::codec::ExecutionReport> const &event) {
  // XXX FIXME TODO HANS implement
}

// tools

template <typename T>
void Controller::dispatch(Trace<T> const &event, uint64_t session_id) {
  session_manager_.get_session(session_id, [&](auto &session) { session(event); });
}

}  // namespace fix_bridge
}  // namespace roq
