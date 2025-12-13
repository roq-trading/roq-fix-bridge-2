/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/fix_bridge/application.hpp"

#include <vector>

#include "roq/client.hpp"
#include "roq/logging.hpp"

#include "roq/fix_bridge/config.hpp"
#include "roq/fix_bridge/controller.hpp"

#include "roq/fix_bridge/settings.hpp"

using namespace std::literals;

namespace roq {
namespace fix_bridge {

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  auto params = args.params();
  if (std::empty(params)) {
    log::fatal("Expected arguments"sv);
  }
  Settings settings{args};
  Config config{settings};
  log::info("Starting the bridge"sv);
  client::Bridge{settings, config, params}.dispatch<Controller>(settings, config, std::size(params));
  return EXIT_SUCCESS;
}

}  // namespace fix_bridge
}  // namespace roq
