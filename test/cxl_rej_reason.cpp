/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/logging.hpp"  // DEBUG

#include "roq/utils/debug/fix/message.hpp"

#include "roq/fix/codec/encoder.hpp"

using namespace std::literals;
using namespace std::chrono_literals;  // NOLINT

using namespace Catch::literals;

using namespace roq;

// === IMPLEMENTATION ===

TEST_CASE("order_cancel_reject", "[order_cancel_reject]") {
  auto header = fix::Header{
      .version = fix::Version::FIX_44,
      .msg_type = fix::MsgType::ORDER_CANCEL_REJECT,
      .sender_comp_id = "from"sv,
      .target_comp_id = "to"sv,
      .msg_seq_num = 1,
      .sending_time = 123ns,
  };
  {  // normal
    auto order_cancel_reject = fix::codec::OrderCancelReject{
        .order_id = "123"sv,
        .secondary_cl_ord_id = {},
        .cl_ord_id = "123"sv,
        .orig_cl_ord_id = "123"sv,
        .ord_status = fix::OrdStatus::NEW,
        .working_indicator = true,
        .account = "A1"sv,
        .cxl_rej_response_to = fix::CxlRejResponseTo::ORDER_CANCEL_REPLACE_REQUEST,
        .cxl_rej_reason = fix::CxlRejReason::OTHER,  // note!
        .text = "some message"sv,
    };
    auto encoder = fix::codec::Encoder::create();
    auto message = (*encoder).encode(header, order_cancel_reject);
    auto message_2 = fmt::format("{}"sv, utils::debug::fix::Message{message});
    CHECK(
        message_2 ==
        "8=FIX.4.4|9=0000115|35=9|49=from|56=to|34=1|52=19700101-00:00:00.000|37=123|11=123|41=123|39=0|636=Y|1=A1|434=2|102=99|58=some message|10=150|"sv);
  }
  {  // special value
    auto order_cancel_reject = fix::codec::OrderCancelReject{
        .order_id = "123"sv,
        .secondary_cl_ord_id = {},
        .cl_ord_id = "123"sv,
        .orig_cl_ord_id = "123"sv,
        .ord_status = fix::OrdStatus::NEW,
        .working_indicator = true,
        .account = "A1"sv,
        .cxl_rej_response_to = fix::CxlRejResponseTo::ORDER_CANCEL_REPLACE_REQUEST,
        .cxl_rej_reason = static_cast<fix::CxlRejReason>(1139),  // note!
        .text = "some message"sv,
    };
    auto encoder = fix::codec::Encoder::create();
    auto message = (*encoder).encode(header, order_cancel_reject);
    auto message_2 = fmt::format("{}"sv, utils::debug::fix::Message{message});
    CHECK(
        message_2 ==
        "8=FIX.4.4|9=0000117|35=9|49=from|56=to|34=1|52=19700101-00:00:00.000|37=123|11=123|41=123|39=0|636=Y|1=A1|434=2|102=1139|58=some message|10=244|"sv);
  }
}
