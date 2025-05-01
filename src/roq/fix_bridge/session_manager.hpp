/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <memory>
#include <utility>

#include "roq/api.hpp"

#include "roq/utils/container.hpp"

#include "roq/io/context.hpp"
#include "roq/io/net/tcp/listener.hpp"
#include "roq/io/sys/timer.hpp"

#include "roq/fix_bridge/session.hpp"
#include "roq/fix_bridge/shared.hpp"

namespace roq {
namespace fix_bridge {

struct SessionManager final : public io::sys::Timer::Handler, public io::net::tcp::Listener::Handler, public Session::Handler {
  explicit SessionManager(Shared &);

  SessionManager(SessionManager &&) = delete;
  SessionManager(SessionManager const &) = delete;

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  // tools

  template <typename Callback>
  bool get_session(uint64_t session_id, Callback callback) const {
    auto iter = sessions_.find(session_id);
    if (iter == std::end(sessions_))
      return false;
    callback(*(*iter).second);
    return true;
  }

  template <typename Callback>
  void get_all_sessions(Callback callback) const {
    for (auto &[session_id, session] : sessions_)
      callback(*session);
  }

 protected:
  void run();

  // io::sys::Timer

  void operator()(io::sys::Timer::Event const &) override;

  // io::net::tcp::Listener::Handler

  void operator()(io::net::tcp::Connection::Factory &) override;
  void operator()(io::net::tcp::Connection::Factory &, io::NetworkAddress const &) override;

  void helper(io::net::tcp::Connection::Factory &, uint64_t session_id);

  // Session::Handler

  void operator()(Session::Disconnect const &) override;

  // tools

  void add_session(std::unique_ptr<Session> &&);
  void remove_session(uint64_t session_id);

  void remove_zombies();

 private:
  // io
  std::unique_ptr<io::Context> const context_;
  std::unique_ptr<io::sys::Timer> const timer_;
  std::unique_ptr<io::net::tcp::Listener> const listener_;
  // shared
  Shared &shared_;
  // sessions
  uint64_t next_session_id_ = {};
  utils::unordered_map<uint64_t, std::unique_ptr<Session>> sessions_;
  std::chrono::nanoseconds next_cleanup_ = {};
  utils::unordered_set<uint64_t> zombies_;
};

}  // namespace fix_bridge
}  // namespace roq
