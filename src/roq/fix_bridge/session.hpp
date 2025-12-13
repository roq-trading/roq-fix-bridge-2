/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>
#include <vector>

#include "roq/trace.hpp"

#include "roq/io/buffer.hpp"
#include "roq/io/net/tcp/connection.hpp"

#include "roq/fix/codec/error.hpp"

#include "roq/fix/codec/heartbeat.hpp"
#include "roq/fix/codec/logon.hpp"
#include "roq/fix/codec/logout.hpp"
#include "roq/fix/codec/reject.hpp"
#include "roq/fix/codec/resend_request.hpp"
#include "roq/fix/codec/test_request.hpp"

#include "roq/fix/codec/security_definition_request.hpp"
#include "roq/fix/codec/security_list_request.hpp"
#include "roq/fix/codec/security_status_request.hpp"

#include "roq/fix/codec/market_data_request.hpp"

#include "roq/fix/codec/user_request.hpp"

#include "roq/fix/codec/new_order_single.hpp"
#include "roq/fix/codec/order_cancel_replace_request.hpp"
#include "roq/fix/codec/order_cancel_request.hpp"
#include "roq/fix/codec/order_mass_cancel_request.hpp"
#include "roq/fix/codec/order_mass_status_request.hpp"
#include "roq/fix/codec/order_status_request.hpp"

#include "roq/fix/codec/trade_capture_report_request.hpp"
#include "roq/fix/codec/trading_session_status_request.hpp"

#include "roq/fix/codec/request_for_positions.hpp"

#include "roq/fix/codec/mass_quote.hpp"
#include "roq/fix/codec/quote_cancel.hpp"

#include "roq/fix/codec/market_data_incremental_refresh.hpp"
#include "roq/fix/codec/market_data_request_reject.hpp"
#include "roq/fix/codec/market_data_snapshot_full_refresh.hpp"
#include "roq/fix/codec/security_definition.hpp"
#include "roq/fix/codec/security_list.hpp"
#include "roq/fix/codec/security_status.hpp"
#include "roq/fix/codec/trading_session_status.hpp"

#include "roq/fix/codec/execution_report.hpp"
#include "roq/fix/codec/order_cancel_reject.hpp"
#include "roq/fix/codec/order_mass_cancel_report.hpp"
#include "roq/fix/codec/position_report.hpp"
#include "roq/fix/codec/request_for_positions_ack.hpp"
#include "roq/fix/codec/trade_capture_report.hpp"
#include "roq/fix/codec/trade_capture_report_request_ack.hpp"

#include "roq/fix/bridge/manager.hpp"

namespace roq {
namespace fix_bridge {

struct Shared;  // circular

struct Session final : public io::net::tcp::Connection::Handler {
  struct Disconnect final {
    uint64_t session_id = {};
  };

  struct Handler {
    virtual void operator()(Disconnect const &) = 0;
  };

  Session(Handler &, uint64_t session_id, io::net::tcp::Connection::Factory &, Shared &);

  Session(Session &&) = delete;
  Session(Session const &) = delete;

  uint64_t get_session_id() const { return session_id_; }

  void write(std::span<std::byte const> const &);

  void logout(fix::codec::Error);

  // from client

  void operator()(Trace<fix::codec::Logon> const &, fix::Header const &);
  void operator()(Trace<fix::codec::Logout> const &, fix::Header const &);
  void operator()(Trace<fix::codec::TestRequest> const &, fix::Header const &);
  void operator()(Trace<fix::codec::ResendRequest> const &, fix::Header const &);
  void operator()(Trace<fix::codec::Reject> const &, fix::Header const &);
  void operator()(Trace<fix::codec::Heartbeat> const &, fix::Header const &);

  void operator()(Trace<fix::codec::TradingSessionStatusRequest> const &, fix::Header const &);

  void operator()(Trace<fix::codec::SecurityListRequest> const &, fix::Header const &);
  void operator()(Trace<fix::codec::SecurityDefinitionRequest> const &, fix::Header const &);
  void operator()(Trace<fix::codec::SecurityStatusRequest> const &, fix::Header const &);
  void operator()(Trace<fix::codec::MarketDataRequest> const &, fix::Header const &);

  void operator()(Trace<fix::codec::UserRequest> const &, fix::Header const &);

  void operator()(Trace<fix::codec::OrderStatusRequest> const &, fix::Header const &);
  void operator()(Trace<fix::codec::OrderMassStatusRequest> const &, fix::Header const &);
  void operator()(Trace<fix::codec::NewOrderSingle> const &, fix::Header const &);
  void operator()(Trace<fix::codec::OrderCancelRequest> const &, fix::Header const &);
  void operator()(Trace<fix::codec::OrderCancelReplaceRequest> const &, fix::Header const &);
  void operator()(Trace<fix::codec::OrderMassCancelRequest> const &, fix::Header const &);

  void operator()(Trace<fix::codec::TradeCaptureReportRequest> const &, fix::Header const &);

  void operator()(Trace<fix::codec::RequestForPositions> const &, fix::Header const &);

  void operator()(Trace<fix::codec::MassQuote> const &, fix::Header const &);
  void operator()(Trace<fix::codec::QuoteCancel> const &, fix::Header const &);

  // to client

  void operator()(Trace<fix::bridge::Manager::Disconnect> const &);
  void operator()(Trace<fix::codec::Reject> const &);
  void operator()(Trace<fix::codec::Logon> const &);
  void operator()(Trace<fix::codec::Logout> const &);
  void operator()(Trace<fix::codec::Heartbeat> const &);
  void operator()(Trace<fix::codec::TestRequest> const &);
  void operator()(Trace<fix::codec::BusinessMessageReject> const &);
  void operator()(Trace<fix::codec::UserResponse> const &);
  void operator()(Trace<fix::codec::TradingSessionStatus> const &);
  void operator()(Trace<fix::codec::SecurityList> const &);
  void operator()(Trace<fix::codec::SecurityDefinition> const &);
  void operator()(Trace<fix::codec::SecurityStatus> const &);
  void operator()(Trace<fix::codec::MarketDataRequestReject> const &);
  void operator()(Trace<fix::codec::MarketDataSnapshotFullRefresh> const &);
  void operator()(Trace<fix::codec::MarketDataIncrementalRefresh> const &);
  void operator()(Trace<fix::codec::ExecutionReport> const &);
  void operator()(Trace<fix::codec::OrderCancelReject> const &);
  void operator()(Trace<fix::codec::OrderMassCancelReport> const &);
  void operator()(Trace<fix::codec::TradeCaptureReportRequestAck> const &);
  void operator()(Trace<fix::codec::TradeCaptureReport> const &);
  void operator()(Trace<fix::codec::RequestForPositionsAck> const &);
  void operator()(Trace<fix::codec::PositionReport> const &);
  void operator()(Trace<fix::codec::MassQuoteAck> const &);
  void operator()(Trace<fix::codec::QuoteStatusReport> const &);

 protected:
  void operator()(io::net::tcp::Connection::Read const &) override;
  void operator()(io::net::tcp::Connection::Disconnected const &) override;

  // fix

  void check(fix::Header const &);

  void parse(Trace<fix::Message> const &);
  void parse_helper(Trace<fix::Message> const &);

  template <std::size_t level, typename T>
  void send_and_close(T const &);

  template <std::size_t level, typename T>
  void send(T const &);

  template <std::size_t level, typename T>
  void send_maybe_override_sending_time(T const &, std::chrono::nanoseconds exchange_time_utc);

  //

  template <std::size_t level, typename T>
  void send(T const &, MessageInfo const &);

  template <std::size_t level, typename T>
  void send_maybe_override_sending_time(T const &, MessageInfo const &, std::chrono::nanoseconds exchange_time_utc);

  //

  template <std::size_t level, typename T>
  void send_helper(T const &, std::chrono::nanoseconds sending_time, std::chrono::nanoseconds origin_create_time = {});

  void close();

  void make_zombie();

  void unsubscribe_all();

  void cancel_all_orders();

  template <typename T>
  void dispatch(Trace<T> const &, fix::Header const &);

 private:
  Handler &handler_;
  // config
  uint64_t const session_id_;
  std::string const name_;
  std::string const prefix_;
  // io
  std::unique_ptr<io::net::tcp::Connection> connection_;
  io::Buffer buffer_;
  // shared
  Shared &shared_;
  // state
  enum class State { WAITING_LOGON, READY, ZOMBIE } state_ = {};
  std::string account_;
  uint16_t user_id_ = {};
  bool waiting_for_heartbeat_ = false;
  // --- fix ---
  // buffer
  std::vector<std::byte> decode_buffer_;
  std::vector<std::byte> decode_buffer_2_;
  std::vector<std::byte> encode_buffer_;
  // state
  struct {
    uint64_t msg_seq_num = {};
  } outbound_;
  struct {
    uint64_t msg_seq_num = {};
  } inbound_;
  std::string comp_id_;
};

}  // namespace fix_bridge
}  // namespace roq
