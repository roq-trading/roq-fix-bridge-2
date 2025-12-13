/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/fix_bridge/session_manager.hpp"

#include <algorithm>
#include <memory>
#include <utility>

#include <cassert>
#include <chrono>
#include <filesystem>
#include <string>

#include "roq/logging.hpp"

#include "roq/clock.hpp"

#include "roq/utils/charconv/from_chars.hpp"

#include "roq/io/network_address.hpp"

#include "roq/io/engine/context_factory.hpp"

using namespace std::literals;
using namespace std::chrono_literals;

namespace roq {
namespace fix_bridge {

// === CONSTANTS ===

namespace {
auto const CLEANUP_FREQUENCY = 1s;
}

// === HELPERS ===

namespace {
auto create_context() {
  return io::engine::ContextFactory::create();
}

auto create_network_address(auto &settings) {
  auto address = settings.common.client_listen_address;
  uint16_t port = {};
  if (utils::charconv::try_parse<uint16_t>(address, [&](auto value) { port = value; })) {
    if (port == 0) {
      log::fatal("Unexpected listen port={}"sv, port);
    }
    log::info(R"(The service will be started on port={})"sv, port);
    return io::NetworkAddress{port};
  }
  log::info(R"(The service will be started on path="{}")"sv, address);
  auto directory = std::filesystem::path{address}.parent_path();
  if (!std::empty(directory) && std::filesystem::create_directory(directory)) {
    log::info(R"(Created path="{}")"sv, directory.c_str());
  }
  return io::NetworkAddress{address};
}
}  // namespace

// === IMPLEMENTATION ===

SessionManager::SessionManager(Shared &shared)
    : context_{create_context()}, timer_{(*context_).create_timer(*this, 100ms)},
      listener_{(*context_).create_tcp_listener(*this, create_network_address(shared.settings))}, shared_{shared} {
}

void SessionManager::operator()(Event<Start> const &) {
  (*timer_).resume();
}

void SessionManager::operator()(Event<Stop> const &) {
}

void SessionManager::operator()(Event<Timer> const &) {
  (*context_).drain();
}

void SessionManager::run() {
}

// io::sys::Timer

void SessionManager::operator()(io::sys::Timer::Event const &event) {
  auto now = event.now;
  if (next_cleanup_ < now) {
    next_cleanup_ = now + CLEANUP_FREQUENCY;
    remove_zombies();
  }
}

// io::net::tcp::Listener::Handler

void SessionManager::operator()(io::net::tcp::Connection::Factory &factory) {
  auto session_id = ++next_session_id_;
  log::info("Connected (session_id={})"sv, session_id);
  helper(factory, session_id);
}

void SessionManager::operator()(io::net::tcp::Connection::Factory &factory, io::NetworkAddress const &network_address) {
  auto session_id = ++next_session_id_;
  log::info("Connected (session_id={}, peer={})"sv, session_id, network_address.to_string_2());
  helper(factory, session_id);
}

void SessionManager::helper(io::net::tcp::Connection::Factory &factory, uint64_t session_id) {
  auto session = std::make_unique<Session>(*this, session_id, factory, shared_);
  add_session(std::move(session));
  TraceInfo trace_info;
  fix::bridge::Manager::Connected connected;
  create_trace_and_dispatch(shared_.bridge, trace_info, connected, session_id);
}

// Session::Handler

void SessionManager::operator()(Session::Disconnect const &disconnect) {
  log::info("Detected zombie session"sv);
  shared_.session_logout(disconnect.session_id);
  zombies_.emplace(disconnect.session_id);
}

// tools

void SessionManager::add_session(std::unique_ptr<Session> &&session) {
  auto session_id = (*session).get_session_id();
  [[maybe_unused]] auto res = sessions_.emplace(session_id, std::move(session));
  assert(res.second);
}

void SessionManager::remove_session(uint64_t session_id) {
  [[maybe_unused]] auto count = sessions_.erase(session_id);
  assert(count > 0);
}

void SessionManager::remove_zombies() {
  auto count = std::size(zombies_);
  if (count == 0) {
    return;
  }
  for (auto iter : zombies_) {
    remove_session(iter);
  }
  zombies_.clear();
  log::info("Removed {} zombied session(s) (remaining: {})"sv, count, std::size(sessions_));
}

}  // namespace fix_bridge
}  // namespace roq
