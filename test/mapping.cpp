/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/utils/compare.hpp"

#include "roq/fix_bridge/tools/mapping.hpp"

using namespace std::literals;

using namespace Catch::literals;

using namespace roq;
using namespace roq::fix_bridge;

namespace {
auto const EXCHANGE = "deribit"sv;
auto const INDEX = "BTC-DERIBIT-INDEX"sv;
auto const PERPETUAL = "BTC-PERPETUAL"sv;
auto const FUTURES = "BTC-22JAN22"sv;
auto const OTHER = "BTCUSDT"sv;
}  // namespace

TEST_CASE("mapping_simple", "[mapping]") {
  tools::Mapping mapping{"test"sv, EXCHANGE, "^(\\w+)-DERIBIT-INDEX$"sv, "^(\\w+)-PERPETUAL$"sv};
  CHECK(mapping.add(EXCHANGE, PERPETUAL) == true);
  CHECK(mapping.add(EXCHANGE, INDEX) == true);
  CHECK(mapping.add(EXCHANGE, FUTURES) == false);
  CHECK(mapping.add(EXCHANGE, OTHER) == false);
  CHECK(false == mapping.dispatch(EXCHANGE, PERPETUAL, []([[maybe_unused]] auto &exchange, [[maybe_unused]] auto &symbol) { FAIL_CHECK(); }));
  auto found = false;
  CHECK(true == mapping.dispatch(EXCHANGE, INDEX, [&](auto &exchange, auto &symbol) {
    found = true;
    CHECK(exchange == EXCHANGE);
    CHECK(symbol == PERPETUAL);
  }));
  CHECK(found == true);
  CHECK(false == mapping.dispatch(EXCHANGE, FUTURES, []([[maybe_unused]] auto &exchange, [[maybe_unused]] auto &symbol) { FAIL(); }));
  CHECK(false == mapping.dispatch(EXCHANGE, OTHER, []([[maybe_unused]] auto &exchange, [[maybe_unused]] auto &symbol) { FAIL(); }));
}
