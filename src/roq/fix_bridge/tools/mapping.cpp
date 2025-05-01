/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/fix_bridge/tools/mapping.hpp"

#include "roq/logging.hpp"

#include "roq/utils/compare.hpp"

using namespace std::literals;

namespace roq {
namespace fix_bridge {
namespace tools {

// === HELPERS ===

namespace {
auto create_regex(std::string_view const &regex) {
  std::vector<utils::regex::Pattern> result;
  utils::regex::Pattern regex_2{regex};
  result.emplace_back(std::move(regex_2));
  return result;
}
auto create_regex(std::span<std::string const> const &regex) {
  std::vector<utils::regex::Pattern> result;
  for (auto &iter : regex) {
    utils::regex::Pattern regex_2{iter};
    result.emplace_back(std::move(regex_2));
  }
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

Mapping::Mapping(std::string_view const &name, std::string_view const &exchange, std::string_view const &source_regex, std::string_view const &targets_regex)
    : name_(name), exchange_(exchange), source_regex_{source_regex}, targets_regex_(create_regex(targets_regex)) {
}

Mapping::Mapping(
    std::string_view const &name, std::string_view const &exchange, std::string_view const &source_regex, std::span<std::string const> const &targets_regex)
    : name_(name), exchange_(exchange), source_regex_{source_regex}, targets_regex_(create_regex(targets_regex)) {
}

bool Mapping::add(std::string_view const &exchange, std::string_view const &symbol) {
  if (exchange_ != exchange)
    return false;
  // debug
  auto res = processed_.emplace(symbol);
  if (!res.second)
    log::fatal(R"(Unexpected: symbol="{}")"sv, symbol);
  // regex matching
  auto result = false;
  // source
  {
    auto [whole, key] = source_regex_.extract(symbol);
    if (whole) {
      add_source(key, symbol);
      result = true;
    }
    /*
    std::cmatch match;
    std::regex_match(std::begin(symbol), std::end(symbol), match, source_regex_);
    if (std::size(match) == 2) {
      std::string key(match[1]);
      add_source(key, symbol);
      result = true;
    }
    */
  }
  // target
  for (auto &regex : targets_regex_) {
    auto [whole, key] = regex.extract(symbol);
    if (whole) {
      add_target(key, symbol);
      result = true;
    }
    /*
    std::cmatch match;
    std::regex_match(std::begin(symbol), std::end(symbol), match, regex);
    if (std::size(match) == 2) {
      std::string key(match[1]);
      add_target(key, symbol);
      result = true;
    }
    */
  }
  return result;
}

void Mapping::add_source(std::string_view const &key, std::string_view const &symbol) {
  // log::info(R"(DEBUG: source=("{}" --> "{}"))"sv, key, symbol);
  auto res = source_.emplace(key, symbol);
  if (!res.second)
    log::fatal(R"(Multiple source mappings key="{}", symbol=(new="{}", have="{}"))"sv, key, symbol, (*res.first).second);
  auto iter = target_.find(key);
  if (iter != std::end(target_)) {
    auto &outbound = outbound_[symbol];
    auto &symbols = (*iter).second;
    for (auto &target : symbols) {
      // log::info(R"(DEBUG: outbound("{}" --> "{}"))"sv, symbol, target);
      [[maybe_unused]] auto res = outbound.emplace(target);
      assert(res.second);
    }
  }
}

void Mapping::add_target(std::string_view const &key, std::string_view const &symbol) {
  // log::info(R"(DEBUG: target=("{}" --> "{}"))"sv, key, symbol);
  auto &target = target_[key];
  auto res = target.emplace(symbol);
  if (!res.second)
    log::fatal(R"("Multiple insertions key="{}", symbol="{}")"sv, key, symbol);
  auto iter = source_.find(key);
  if (iter != std::end(source_)) {
    auto &source = (*iter).second;
    auto &outbound = outbound_[source];
    log::info(R"(DEBUG: outbound=("{}" --> "{}"))"sv, source, symbol);
    [[maybe_unused]] auto res = outbound.emplace(symbol);
    assert(res.second);
  }
}

}  // namespace tools
}  // namespace fix_bridge
}  // namespace roq
