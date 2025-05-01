/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <memory>

#include "roq/api.hpp"
#include "roq/client.hpp"

#include "roq/fix/bridge/manager.hpp"

#include "roq/fix_bridge/config.hpp"
#include "roq/fix_bridge/session_manager.hpp"
#include "roq/fix_bridge/settings.hpp"
#include "roq/fix_bridge/shared.hpp"

#include "roq/fix_bridge/tools/crypto.hpp"

namespace roq {
namespace fix_bridge {

struct Controller final : public client::Handler, public fix::bridge::Manager::Handler {
  Controller(client::Dispatcher &, Settings const &, Config const &, size_t source_count);

  Controller(Controller &&) = delete;
  Controller(Controller const &) = delete;

 protected:
  // client

  void operator()(Event<Start> const &) override;
  void operator()(Event<Stop> const &) override;
  void operator()(Event<Timer> const &) override;

  void operator()(Event<Connected> const &) override;
  void operator()(Event<Disconnected> const &) override;

  void operator()(Event<Control> const &) override;

  void operator()(Event<DownloadBegin> const &) override;
  void operator()(Event<DownloadEnd> const &) override;
  void operator()(Event<Ready> const &) override;

  void operator()(Event<GatewaySettings> const &) override;

  void operator()(Event<StreamStatus> const &) override;

  void operator()(Event<GatewayStatus> const &) override;

  void operator()(Event<ReferenceData> const &) override;
  void operator()(Event<MarketStatus> const &) override;
  void operator()(Event<TopOfBook> const &) override;
  void operator()(Event<MarketByPriceUpdate> const &) override;
  void operator()(Event<TradeSummary> const &) override;
  void operator()(Event<StatisticsUpdate> const &) override;

  void operator()(Event<OrderAck> const &) override;
  void operator()(Event<OrderUpdate> const &) override;
  void operator()(Event<TradeUpdate> const &) override;

  void operator()(Event<FundsUpdate> const &) override;
  void operator()(Event<PositionUpdate> const &) override;

  void operator()(metrics::Writer &) const override;

  // bridge

  std::pair<fix::codec::Error, std::string_view> operator()(fix::bridge::Manager::Credentials const &) override;

  void operator()(CreateOrder const &, uint8_t source) override;
  void operator()(ModifyOrder const &, uint8_t source) override;
  void operator()(CancelOrder const &, uint8_t source) override;
  void operator()(CancelAllOrders const &) override;

  void operator()(roq::MassQuote const &, uint8_t source) override;
  void operator()(CancelQuotes const &, uint8_t source) override;

  void operator()(Trace<fix::bridge::Manager::Disconnect> const &, uint64_t session_id) override;

  void operator()(Trace<fix::codec::Reject> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::Logon> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::Logout> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::Heartbeat> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::TestRequest> const &, uint64_t session_id) override;

  void operator()(Trace<fix::codec::BusinessMessageReject> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::UserResponse> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::TradingSessionStatus> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::SecurityList> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::SecurityDefinition> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::SecurityStatus> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::MarketDataRequestReject> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::MarketDataSnapshotFullRefresh> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::MarketDataIncrementalRefresh> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::ExecutionReport> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::OrderCancelReject> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::OrderMassCancelReport> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::TradeCaptureReportRequestAck> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::TradeCaptureReport> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::RequestForPositionsAck> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::PositionReport> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::MassQuoteAck> const &, uint64_t session_id) override;
  void operator()(Trace<fix::codec::QuoteStatusReport> const &, uint64_t session_id) override;

  void operator()(Trace<fix::codec::ExecutionReport> const &) override;

  // tools

  template <typename T>
  void dispatch(Trace<T> const &, uint64_t session_id);

 private:
  client::Dispatcher &dispatcher_;
  std::unique_ptr<fix::bridge::Manager> bridge_;
  Shared shared_;
  SessionManager session_manager_;
  utils::unordered_map<std::string, User> username_to_user_;
  tools::Crypto crypto_;
};

}  // namespace fix_bridge
}  // namespace roq
