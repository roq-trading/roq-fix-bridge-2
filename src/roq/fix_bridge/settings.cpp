/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/fix_bridge/settings.hpp"

#include "roq/logging.hpp"

#include "roq/client/flags/settings.hpp"

using namespace std::literals;

namespace roq {
namespace fix_bridge {

Settings::Settings(args::Parser const &args)
    : client::flags::Settings{args}, common{flags::Common::create()}, fix{flags::FIX::create()}, messaging{flags::Messaging::create()},
      oms{flags::OMS::create()}, test{flags::Test::create()} {
  log::info("settings={}"sv, *this);
}

}  // namespace fix_bridge
}  // namespace roq
