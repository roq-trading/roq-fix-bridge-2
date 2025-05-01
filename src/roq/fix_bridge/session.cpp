/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/fix_bridge/session.hpp"

#include "roq/logging.hpp"

#include "roq/utils/update.hpp"

#include "roq/utils/charconv/from_chars.hpp"

#include "roq/utils/exceptions/unhandled.hpp"

#include "roq/fix/reader.hpp"

#include "roq/fix/codec/business_message_reject.hpp"
#include "roq/fix/codec/user_response.hpp"

#include "roq/fix_bridge/shared.hpp"

using namespace std::literals;

namespace roq {
namespace fix_bridge {

// === CONSTANTS ===

namespace {
auto const FIX_VERSION = fix::Version::FIX_44;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto session_id) {
  auto const NAME = "cl"sv;
  return fmt::format("{}:{}"sv, session_id, NAME);
}

auto create_prefix(auto session_id) {
  return fmt::format("[session_id={}]:"sv, session_id);
}

auto get_text(fix::codec::Error error) -> std::string_view {
  switch (error) {
    using enum fix::codec::Error;
    case UNDEFINED:
      return {};
    case NO_ORDERS:
      return "NO ORDERS"sv;
    case NO_TRADES:
      return "NO TRADES"sv;
    default:
      break;
  }
  return magic_enum::enum_name(error);
}

auto get_strategy_id_helper(auto &value) {
  return utils::charconv::from_chars<uint32_t>(value);
}
}  // namespace

// === IMPLEMENTATION ===

Session::Session(Handler &handler, uint64_t session_id, io::net::tcp::Connection::Factory &factory, Shared &shared)
    : handler_{handler}, session_id_{session_id}, name_{create_name(session_id)}, prefix_{create_prefix(session_id)}, connection_{factory.create(*this)},
      shared_{shared}, decode_buffer_(shared.settings.fix.fix_decode_buffer_size), decode_buffer_2_(shared.settings.fix.fix_decode_buffer_size),
      encode_buffer_(shared.settings.fix.fix_encode_buffer_size) {
}

void Session::logout(fix::codec::Error error) {
  auto logout = fix::codec::Logout{
      .text = get_text(error),
  };
  send_and_close<2>(logout);
}

void Session::operator()(io::net::tcp::Connection::Read const &) {
  if (state_ == State::ZOMBIE)
    return;
  buffer_.append(*connection_);
  auto buffer = buffer_.data();
  try {
    size_t total_bytes = 0;
    auto parser = [&](auto &message) {
      // note! capture receive_time and origin_create_time
      shared_.dispatcher.create_trace_info([&]([[maybe_unused]] auto &trace_info) {
        check(message.header);
        Trace event{trace_info, message};
        parse(event);
      });
    };
    auto logger = [&](auto &message) { shared_.fix_log.received<1>(session_id_, message); };
    while (!std::empty(buffer)) {
      auto bytes = fix::Reader<FIX_VERSION>::dispatch(buffer, parser, logger);
      if (bytes == 0)
        break;
      assert(bytes <= std::size(buffer));
      total_bytes += bytes;
      buffer = buffer.subspan(bytes);
      if (state_ == State::ZOMBIE)
        break;
    }
    buffer_.drain(total_bytes);
  } catch (SystemError &e) {
    log::error("{} Exception: {}"sv, prefix_, e);
    close();
  } catch (Exception &e) {
    log::error("{} Exception: {}"sv, prefix_, e);
    close();
  } catch (std::exception &e) {
    log::error("{} Exception: {}"sv, prefix_, e.what());
    close();
  } catch (...) {
    utils::exceptions::Unhandled::terminate();
  }
}

void Session::operator()(io::net::tcp::Connection::Disconnected const &) {
  make_zombie();
  TraceInfo trace_info;
  auto disconnected = fix::bridge::Manager::Disconnected{};
  create_trace_and_dispatch(shared_.bridge, trace_info, disconnected, session_id_);
}

void Session::check(fix::Header const &header) {
  auto current = header.msg_seq_num;
  auto expected = inbound_.msg_seq_num + 1;
  if (current != expected) [[unlikely]] {
    if (expected < current) {
      log::warn(
          "{} "
          "*** SEQUENCE GAP *** "
          "current={} previous={} distance={}"sv,
          prefix_,
          current,
          inbound_.msg_seq_num,
          current - inbound_.msg_seq_num);
    } else {
      log::warn(
          "{} "
          "*** SEQUENCE REPLAY *** "
          "current={} previous={} distance={}"sv,
          prefix_,
          current,
          inbound_.msg_seq_num,
          inbound_.msg_seq_num - current);
    }
  }
  inbound_.msg_seq_num = current;
}

void Session::parse(Trace<fix::Message> const &event) {
  shared_.profile.parse([&]() { parse_helper(event); });
}

void Session::parse_helper(Trace<fix::Message> const &event) {
  auto &[trace_info, message] = event;
  switch (message.header.msg_type) {
    using enum fix::MsgType;
    // session
    case LOGON:
      shared_.profile.logon([&]() {
        auto logon = fix::codec::Logon::create(message);
        create_trace_and_dispatch(*this, trace_info, logon, message.header);
      });
      break;
    case LOGOUT:
      shared_.profile.logout([&]() {
        auto logout = fix::codec::Logout::create(message);
        create_trace_and_dispatch(*this, trace_info, logout, message.header);
      });
      break;
    case TEST_REQUEST:
      shared_.profile.test_request([&]() {
        auto test_request = fix::codec::TestRequest::create(message);
        create_trace_and_dispatch(*this, trace_info, test_request, message.header);
      });
      break;
    case RESEND_REQUEST:
      shared_.profile.resend_request([&]() {
        auto resend_request = fix::codec::ResendRequest::create(message);
        create_trace_and_dispatch(*this, trace_info, resend_request, message.header);
      });
      break;
    case REJECT:
      shared_.profile.reject([&]() {
        auto reject = fix::codec::Reject::create(message);
        create_trace_and_dispatch(*this, trace_info, reject, message.header);
      });
      break;
    case HEARTBEAT:
      shared_.profile.heartbeat([&]() {
        auto heartbeat = fix::codec::Heartbeat::create(message);
        create_trace_and_dispatch(*this, trace_info, heartbeat, message.header);
      });
      break;
      // business
      // - trading session
    case TRADING_SESSION_STATUS_REQUEST:
      shared_.profile.trading_session_status_request([&]() {
        auto trading_session_status_request = fix::codec::TradingSessionStatusRequest::create(message);
        create_trace_and_dispatch(*this, trace_info, trading_session_status_request, message.header);
      });
      break;
      // - market data
    case SECURITY_LIST_REQUEST:
      shared_.profile.security_list_request([&]() {
        auto security_list_request = fix::codec::SecurityListRequest::create(message);
        create_trace_and_dispatch(*this, trace_info, security_list_request, message.header);
      });
      break;
    case SECURITY_DEFINITION_REQUEST:
      shared_.profile.security_definition_request([&]() {
        auto security_definition_request = fix::codec::SecurityDefinitionRequest::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, security_definition_request, message.header);
      });
      break;
    case SECURITY_STATUS_REQUEST:
      shared_.profile.security_status_request([&]() {
        auto security_status_request = fix::codec::SecurityStatusRequest::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, security_status_request, message.header);
      });
      break;
    case MARKET_DATA_REQUEST:
      shared_.profile.market_data_request([&]() {
        auto market_data_request = fix::codec::MarketDataRequest::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, market_data_request, message.header);
      });
      break;
      // - user management
    case USER_REQUEST:
      shared_.profile.user_request([&]() {
        auto user_request = fix::codec::UserRequest::create(message);
        create_trace_and_dispatch(*this, trace_info, user_request, message.header);
      });
      break;
      // - order management
    case ORDER_STATUS_REQUEST:
      shared_.profile.order_status_request([&]() {
        auto order_status_request = fix::codec::OrderStatusRequest::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, order_status_request, message.header);
      });
      break;
    case ORDER_MASS_STATUS_REQUEST:
      shared_.profile.order_mass_status_request([&]() {
        auto order_mass_status_request = fix::codec::OrderMassStatusRequest::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, order_mass_status_request, message.header);
      });
      break;
    case NEW_ORDER_SINGLE:
      shared_.profile.new_order_single([&]() {
        auto new_order_single = fix::codec::NewOrderSingle::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, new_order_single, message.header);
      });
      break;
    case ORDER_CANCEL_REQUEST:
      shared_.profile.order_cancel_request([&]() {
        auto order_cancel_request = fix::codec::OrderCancelRequest::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, order_cancel_request, message.header);
      });
      break;
    case ORDER_CANCEL_REPLACE_REQUEST:
      shared_.profile.order_cancel_replace_request([&]() {
        auto order_cancel_replace_request = fix::codec::OrderCancelReplaceRequest::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, order_cancel_replace_request, message.header);
      });
      break;
    case ORDER_MASS_CANCEL_REQUEST:
      shared_.profile.order_mass_cancel_request([&]() {
        auto order_mass_cancel_request = fix::codec::OrderMassCancelRequest::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, order_mass_cancel_request, message.header);
      });
      break;
      // - trade capture reporting
    case TRADE_CAPTURE_REPORT_REQUEST:
      shared_.profile.trade_capture_report_request([&]() {
        auto trade_capture_report_request = fix::codec::TradeCaptureReportRequest::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, trade_capture_report_request, message.header);
      });
      break;
      // - position management
    case REQUEST_FOR_POSITIONS:
      shared_.profile.request_for_positions([&]() {
        auto request_for_positions = fix::codec::RequestForPositions::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, request_for_positions, message.header);
      });
      break;
      // - quoting
    case MASS_QUOTE:
      shared_.profile.mass_quote([&]() {
        auto mass_quote = fix::codec::MassQuote::create(message, decode_buffer_, decode_buffer_2_);
        create_trace_and_dispatch(*this, trace_info, mass_quote, message.header);
      });
      break;
    case QUOTE_CANCEL:
      shared_.profile.quote_cancel([&]() {
        auto quote_cancel = fix::codec::QuoteCancel::create(message, decode_buffer_);
        create_trace_and_dispatch(*this, trace_info, quote_cancel, message.header);
      });
      break;
    default: {
      log::warn("{} Unexpected msg_type={}"sv, prefix_, message.header.msg_type);
      auto const error = fix::codec::Error::UNEXPECTED_MSG_TYPE;
      auto response = fix::codec::BusinessMessageReject{
          .ref_seq_num = message.header.msg_seq_num,
          .ref_msg_type = message.header.msg_type,
          .business_reject_ref_id = {},
          .business_reject_reason = fix::BusinessRejectReason::UNSUPPORTED_MESSAGE_TYPE,
          .text = get_text(error),
      };
      send<2>(response);
      break;
    }
  }
}

void Session::operator()(Trace<fix::codec::Logon> const &event, fix::Header const &header) {
  dispatch(event, header);
}

void Session::operator()(Trace<fix::codec::Logout> const &event, fix::Header const &header) {
  dispatch(event, header);
}

void Session::operator()(Trace<fix::codec::TestRequest> const &event, fix::Header const &header) {
  dispatch(event, header);
}

void Session::operator()(Trace<fix::codec::ResendRequest> const &event, fix::Header const &header) {
  dispatch(event, header);
  //
  dispatch(event, header);
  auto &[trace_info, resend_request] = event;
  log::info<1>("{} resend_request={}"sv, prefix_, resend_request);
  switch (state_) {
    using enum State;
    case WAITING_LOGON: {
      auto const error = fix::codec::Error::NO_LOGON;
      auto response = fix::codec::Reject{
          .ref_seq_num = header.msg_seq_num,
          .text = get_text(error),
          .ref_tag_id = {},
          .ref_msg_type = header.msg_type,
          .session_reject_reason = fix::SessionRejectReason::OTHER,
      };
      send_and_close<2>(response);
      break;
    }
    case READY: {
      auto const error = fix::codec::Error::UNSUPPORTED_MSG_TYPE;
      auto response = fix::codec::BusinessMessageReject{
          .ref_seq_num = header.msg_seq_num,
          .ref_msg_type = header.msg_type,
          .business_reject_ref_id = {},
          .business_reject_reason = fix::BusinessRejectReason::UNSUPPORTED_MESSAGE_TYPE,
          .text = get_text(error),
      };
      send<2>(response);
      break;
    }
    case ZOMBIE:
      assert(false);
      break;
  }
}

void Session::operator()(Trace<fix::codec::Reject> const &event, fix::Header const &header) {
  dispatch(event, header);
  //
  auto &[trace_info, reject] = event;
  log::warn("{} reject={}"sv, prefix_, reject);
  close();
}

void Session::operator()(Trace<fix::codec::Heartbeat> const &event, fix::Header const &header) {
  dispatch(event, header);
  //
  auto &[trace_info, heartbeat] = event;
  log::info<1>("{} heartbeat={}"sv, prefix_, heartbeat);
  switch (state_) {
    using enum State;
    case WAITING_LOGON: {
      auto const error = fix::codec::Error::NO_LOGON;
      auto response = fix::codec::Reject{
          .ref_seq_num = header.msg_seq_num,
          .text = get_text(error),
          .ref_tag_id = {},
          .ref_msg_type = header.msg_type,
          .session_reject_reason = fix::SessionRejectReason::OTHER,
      };
      send<2>(response);
      break;
    }
    case READY: {
      waiting_for_heartbeat_ = false;
      break;
    }
    case ZOMBIE:
      assert(false);
      break;
  }
}

void Session::operator()(Trace<fix::codec::TradingSessionStatusRequest> const &event, fix::Header const &header) {
  dispatch(event, header);
}

void Session::operator()(Trace<fix::codec::SecurityListRequest> const &event, fix::Header const &header) {
  dispatch(event, header);
}

void Session::operator()(Trace<fix::codec::SecurityDefinitionRequest> const &event, fix::Header const &header) {
  dispatch(event, header);
}

void Session::operator()(Trace<fix::codec::SecurityStatusRequest> const &event, fix::Header const &header) {
  dispatch(event, header);
}

void Session::operator()(Trace<fix::codec::MarketDataRequest> const &event, fix::Header const &header) {
  dispatch(event, header);
}

// XXX HANS this is *NOT* migrated to roq-fix
void Session::operator()(Trace<fix::codec::UserRequest> const &event, fix::Header const &header) {
  dispatch(event, header);
  //
  auto &[trace_info, user_request] = event;
  log::debug("user_request={}"sv, user_request);
  log::info<1>("{} user_request={}"sv, prefix_, user_request);
  if (!shared_.settings.oms.oms_route_by_strategy) {
    // FIXME reject
  }
  auto strategy_id = get_strategy_id_helper(user_request.username);
  if (!strategy_id) {
    // FIXME reject
  }
  switch (user_request.user_request_type) {
    using enum fix::UserRequestType;
    case UNDEFINED:
    case UNKNOWN:
    case LOG_ON_USER: {
      // XXX TODO validation
      auto success = shared_.add_route(session_id_, strategy_id);
      auto user_response = fix::codec::UserResponse{
          .user_request_id = user_request.user_request_id,
          .username = user_request.username,
          .user_status = success ? fix::UserStatus::LOGGED_IN : fix::UserStatus::OTHER,
          .user_status_text = {},
      };
      send<2>(user_response);
      return;  // note
    }
    case LOG_OFF_USER: {
      // XXX TODO validation
      auto success = shared_.remove_route(session_id_, strategy_id);
      auto user_response = fix::codec::UserResponse{
          .user_request_id = user_request.user_request_id,
          .username = user_request.username,
          .user_status = success ? fix::UserStatus::NOT_LOGGED_IN : fix::UserStatus::OTHER,
          .user_status_text = {},
      };
      send<2>(user_response);
      return;  // note
    }
    case CHANGE_PASSWORD_FOR_USER:
    case REQUEST_INDIVIDUAL_USER_STATUS:
      break;
  }
  auto const error = fix::codec::Error::UNSUPPORTED_USER_REQUEST_TYPE;
  auto user_response = fix::codec::UserResponse{
      .user_request_id = user_request.user_request_id,
      .username = user_request.username,
      .user_status = fix::UserStatus::OTHER,
      .user_status_text = get_text(error),
  };
  send<2>(user_response);
}

void Session::operator()(Trace<fix::codec::OrderStatusRequest> const &event, fix::Header const &header) {
  dispatch(event, header);
}

void Session::operator()(Trace<fix::codec::OrderMassStatusRequest> const &event, fix::Header const &header) {
  dispatch(event, header);
}

void Session::operator()(Trace<fix::codec::NewOrderSingle> const &event, fix::Header const &header) {
  dispatch(event, header);
}

void Session::operator()(Trace<fix::codec::OrderCancelRequest> const &event, fix::Header const &header) {
  dispatch(event, header);
}

void Session::operator()(Trace<fix::codec::OrderCancelReplaceRequest> const &event, fix::Header const &header) {
  dispatch(event, header);
}

void Session::operator()(Trace<fix::codec::OrderMassCancelRequest> const &event, fix::Header const &header) {
  dispatch(event, header);
}

void Session::operator()(Trace<fix::codec::TradeCaptureReportRequest> const &event, fix::Header const &header) {
  dispatch(event, header);
}

void Session::operator()(Trace<fix::codec::RequestForPositions> const &event, fix::Header const &header) {
  dispatch(event, header);
}

void Session::operator()(Trace<fix::codec::MassQuote> const &event, fix::Header const &header) {
  dispatch(event, header);
}

void Session::operator()(Trace<fix::codec::QuoteCancel> const &event, fix::Header const &header) {
  dispatch(event, header);
}

// connection

void Session::operator()(Trace<fix::bridge::Manager::Disconnect> const &) {
  close();
}

// outbound

void Session::operator()(Trace<fix::codec::Reject> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::Logon> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::Logout> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::Heartbeat> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::TestRequest> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::BusinessMessageReject> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::UserResponse> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::TradingSessionStatus> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::SecurityList> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::SecurityDefinition> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::SecurityStatus> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::MarketDataRequestReject> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::MarketDataSnapshotFullRefresh> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::MarketDataIncrementalRefresh> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::ExecutionReport> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::OrderCancelReject> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::OrderMassCancelReport> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::TradeCaptureReportRequestAck> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::TradeCaptureReport> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::RequestForPositionsAck> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::PositionReport> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::MassQuoteAck> const &event) {
  send<2>(event.value);
}

void Session::operator()(Trace<fix::codec::QuoteStatusReport> const &event) {
  send<2>(event.value);
}

namespace {
template <typename T>
constexpr bool is_logout() {
  return false;
}
template <>
constexpr bool is_logout<fix::codec::Logout>() {
  return true;
}
}  // namespace

template <std::size_t level, typename T>
void Session::send_and_close(T const &value) {
  assert(state_ != State::ZOMBIE);
  auto sending_time = clock::get_realtime();
  send_helper<level>(value, sending_time);
  close();
  if constexpr (is_logout<T>()) {
    // XXX here a new flag to delay the next logon being accepted
  }
}

template <std::size_t level, typename T>
void Session::send(T const &value) {
  assert(state_ == State::READY);
  auto sending_time = clock::get_realtime();
  send_helper<level>(value, sending_time);
}

template <std::size_t level, typename T>
void Session::send_maybe_override_sending_time(T const &value, std::chrono::nanoseconds exchange_time_utc) {
  if (exchange_time_utc.count() && shared_.settings.messaging.exchange_time_as_sending_time) {
    send_helper<level>(value, exchange_time_utc);
  } else {
    send<level>(value);
  }
}

template <std::size_t level, typename T>
void Session::send(T const &value, MessageInfo const &message_info) {
  assert(state_ == State::READY);
  auto sending_time = clock::get_realtime();
  send_helper<level>(value, sending_time, message_info.origin_create_time);
}

template <std::size_t level, typename T>
void Session::send_maybe_override_sending_time(T const &value, MessageInfo const &message_info, std::chrono::nanoseconds exchange_time_utc) {
  if (exchange_time_utc.count() && shared_.settings.messaging.exchange_time_as_sending_time) {
    send_helper<level>(value, exchange_time_utc, message_info.origin_create_time);
  } else {
    send<level>(value, message_info);
  }
}

template <std::size_t level, typename T>
void Session::send_helper(T const &value, std::chrono::nanoseconds sending_time, std::chrono::nanoseconds origin_create_time) {
  log::info<level>("{} sending: value={}"sv, prefix_, value);
  assert(!std::empty(comp_id_));
  auto header = fix::Header{
      .version = FIX_VERSION,
      .msg_type = T::MSG_TYPE,
      .sender_comp_id = shared_.settings.fix.fix_comp_id,
      .target_comp_id = comp_id_,
      .msg_seq_num = ++outbound_.msg_seq_num,  // note!
      .sending_time = sending_time,
  };
  if ((*connection_).send([&](auto &buffer) {
        auto message = value.encode(header, buffer);
        shared_.fix_log.sending<level>(session_id_, message);
        return std::size(message);
      })) {
  } else {
    log::warn("HERE"sv);
  }
  if (origin_create_time.count() == 0)
    return;
  auto now = clock::get_system();
  auto latency = now - origin_create_time;
  shared_.latency.end_to_end.update(latency.count());
}

void Session::close() {
  if (state_ != State::ZOMBIE) {
    (*connection_).close();
    make_zombie();
  }
}

void Session::make_zombie() {
  if (state_ == State::READY) {
    unsubscribe_all();
    if (shared_.settings.oms.cancel_on_disconnect)
      cancel_all_orders();
    shared_.remove_all_routes(session_id_);
  }
  if (utils::update(state_, State::ZOMBIE)) {
    auto disconnect = Disconnect{
        .session_id = session_id_,
    };
    handler_(disconnect);
  }
}

void Session::unsubscribe_all() {
  /*
  // md_req_id
  for (auto market_id : md_subscriptions_)
    shared_.get_market(market_id, [&](auto &market) { market.unsubscribe(session_id_); });
  // pos_req_id
  for (auto &[account, tmp_1] : pos_subscriptions_) {
    for (auto &[exchange, tmp_2] : tmp_1) {
      for (auto &item : tmp_2) {
        auto &symbol = item.first;
        auto &pos_req_ids = item.second;
        if (std::empty(exchange)) {
          shared_.find_funds(account, symbol, [&](auto &funds) {
            for (auto &pos_req_id : pos_req_ids)
              funds.unsubscribe(session_id_, pos_req_id);
          });
        } else {
          shared_.find_position(account, exchange, symbol, [&](auto &position) {
            for (auto &pos_req_id : pos_req_ids)
              position.unsubscribe(session_id_, pos_req_id);
          });
        }
      }
    }
  }
  // trade_request_id
  shared_.remove_trade_requests(session_id_);
  */
}

// note! doesn't work with route-by-strategy
void Session::cancel_all_orders() {
  if (std::empty(account_))
    return;
  log::warn(R"({} *** REQUESTING ALL ORDERS TO BE CANCELLED (account="{}") ***)"sv, prefix_, account_);
  auto cancel_all_orders = CancelAllOrders{
      .account = account_,
      .order_id = {},
      .exchange = {},
      .symbol = {},
      .strategy_id = {},
      .side = {},
  };
  shared_.get_sources([&](auto source_id, auto &source_name, bool ready) {
    if (ready) {
      log::warn(R"({} - source_name="{}")"sv, prefix_, source_name);
      try {
        shared_.dispatcher.send(cancel_all_orders, source_id);
      } catch (NotConnected &) {
      }
    }
  });
}

//

template <typename T>
void Session::dispatch(Trace<T> const &event, fix::Header const &header) {
  shared_.bridge(event, header, session_id_);
}

}  // namespace fix_bridge
}  // namespace roq
