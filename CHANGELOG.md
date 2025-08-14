# Change Log

All notable changes will be documented in this file.

## Head

## 1.0.8 &ndash; 2025-08-16

## 1.0.7 &ndash; 2025-07-02

## 1.0.6 &ndash; 2025-05-16

## 1.0.5 &ndash; 2025-03-26

### Added

* TOML config to map Error to CxlRejReason >=100 (#490)

## 1.0.4 &ndash; 2024-12-30

### Changed

* Relax request ID validation (#482)

## 1.0.3 &ndash; 2024-11-26

## 1.0.2 &ndash; 2024-07-14

### Fixed

* `MarketDataRequest` should not allow one-sided requests (#459)

## 1.0.1 &ndash; 2024-04-14

### Changed

* Issues with create limited depth updates (#453)
* Validate Logon fields (#449)

## 1.0.0 &ndash; 2024-03-16

## 0.9.9 &ndash; 2024-01-28

### Fixed

* Invalid `OrdStatus <39>` for `OrderCancelReject <9>` (#426)

### Changed

* Support `TradeSummary.side` (#434)
* TradeCaptureReportRequest now supports filter fields
* RequestForPositions now supports filter fields
* Client's Logon password now being validated (#425)
* Validate ClOrdID (#422)

## 0.9.8 &ndash; 2023-11-20

### Changed

* Preliminary support for by-strategy routing
* Adding filter and parties to `OrderMassCancelRequest` and `OrderMassCancelReport` (#414)
* `OrderMassStatusRequest` to only return working orders
* `OrderStatusRequest` and `OrderMassStatusRequest` not fully FIX 4.4 compliant (#408)

### Fixed

* `OrderMassStatusRequest` didn't return reason `UNKNOWN_ORDER` for no orders. (#418)

## 0.9.7 &ndash; 2023-09-18

### Changed

* `ExecutionReport.order_id` is now matching `OrderAck/OrderUpdate.client_order_id` (#394)
* `ExecutionReport.order_id` now set when rejecting `NewOrderSingle` (#393)
* Allow `MarketDataRequest` with empty `no_md_entry_types` and/or `no_related_sym` when
  `subscription_request_type` is "unsubscribe"
* `strategy_id` now populated based on party id's

## 0.9.6 &ndash; 2023-07-22

### Added

* OrderMassCancelRequest
* OrderMassStatusRequest

### Fixed

* Reject when re-using ClOrdID (NewSingleOrder, OrderCancelRequest, OrderCancelReplaceRequest)

## 0.9.5 &ndash; 2023-06-12

### Changed

* Use `ReferenceData` for FIX price/quantity precision (#367)
* C++ binding for the implemented FIX protocol (#365)

## 0.9.4 &ndash; 2023-05-04

## 0.9.3 &ndash; 2023-03-20

## 0.9.2 &ndash; 2023-02-22

## 0.9.1 &ndash; 2023-01-12

## 0.9.0 &ndash; 2022-12-22

## 0.8.9 &ndash; 2022-11-14

## 0.8.8 &ndash; 2022-10-04

## 0.8.7 &ndash; 2022-08-22

### Changed

* Drop fixed 12 decimal digits (#256)

### Fixed

* MbP dispatching was broken (#263)
* Excessive logging (#250)

## 0.8.6 &ndash; 2022-07-18

### Changed

* return `span<MBPUpdate>` from `cache::MarketByPrice` (#241)
* Support limited-depth MbP (#236)

## 0.8.5 &ndash; 2022-06-06

### Changed

* Experiment: `--test_quantity_multiplier`
* Use MbP decimals (#232)

## 0.8.4 &ndash; 2022-05-14

### Fixed

* VWAP subscription

## 0.8.3 &ndash; 2022-03-22

### Added

* Support custom order book statistics (#209)

### Changed

* Fail on unknown config (toml) keys (#206)
* Make strict checking opt-in (#190)
* Support order download (#38)

### Fixed

* Outbound buffer was not flushed on forced disconnect (#194)
* Exception handler didn't catch protocol error (#184)

## 0.8.2 &ndash; 2022-02-18

### Added

* Broadcast (#167)

## 0.8.1 &ndash; 2022-01-16

## 0.8.0 &ndash; 2022-01-12

### Added

* Flag to control the source of best bid/ask (#151)
* Capture `origin_create_time` for externally triggered events (#140)

### Changed

* MarketDataIncrementalRefresh <X> should use new/changed for MDUpdateAction <279> (#153)

## 0.7.9 &ndash; 2021-12-08

### Added

* Add metrics for `sending_time` - `origin_create_time` (#135)
* Option to pass exchange timestamp to sending time (tag 52) (#125)

### Changed

* Disconnected market data stream will trigger clients to be logged out (workaround) (#132)
* `MarketByPriceUpdate` and `MarketByOrderUpdate` now include price/quantity decimals (#119)
* Log FIX messages at standard verbosity levels (#116)

### Fixed

* `TradeSummary` was not propagated (#136)
* Client sockets did not use `TCP_NODELAY` (#127)
* MDFull and MDInc did not populate `md_entry_date`/`md_entry_time` for MbP updates (#126)
* MbP snapshot should send full refresh (#113)

## 0.7.8 &ndash; 2021-11-02

### Fixed

* Not all update paths populated the `currency` field of `ExecutionReport` (#103)

### Changed

* Move cache utilities to API (#111)
* Remove custom literals (#110)
* Support FundsUpdate (#109)
* ReferenceData currencies should follow FX conventions (#99)
* Using `base_currency` as default for ExecutionReport (8) (#98)

### Added

* Support RequestForPosition <AN>, RequestForPositionAck <AO> and PositionReport <AP> (#92)

## 0.7.7 &ndash; 2021-09-20

### Added

* Cache needed for PositionReport <AP> (#92)
* Flags to terminate on stream and gateway disconnect (#85)
* Temporary command-line flag to silence OrderAck with disconnect or timeout (#71)
* Config file may specify what "zero" actually means (when responding to MD snapshot request) (#62)

### Changed

* Add currency (15) to ExecutionReport (8) (#63)
* Option to terminate on order-ack timeout (#60)

### Fixed

* Temporary work-around to possibly deal with download orders not being supported (#85)
* OrderAck was filtered for missing `routing_id` (#61)


## 0.7.6 &ndash; 2021-09-02

### Added

* New `--init_missing_md_entry_type_to_zero` flag (#45)

### Fixed

* Statistics updates did not respect subscriptions (#42)
* OrderCancelReject did not contain OrdStatus for unknown order (#41)
* Replaced "old" switch statements with mapping lookup (#28)
* Config parser didn't validate StatisticsType overrides (#28)
* Order cancel/replace reject did not reset OrigClOrdID (#22)

## 0.7.5 &ndash; 2021-08-08

## 0.7.4 &ndash; 2021-07-20

## 0.7.3 &ndash; 2021-07-06

## 0.7.2 &ndash; 2021-06-20

### Changed

* Unexpected heartbeats now only cause a warning (previously: logout)

## 0.7.1 &ndash; 2021-05-30
