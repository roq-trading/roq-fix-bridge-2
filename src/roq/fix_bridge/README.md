

drop 1:1 account
----------------

config has list of account regex's
shared doesn't lock user (allow multiple logon)
check `req_id` mappings get cleaned up on completion and disconnect
check all `req_id` mappings live inside the roq-fix library
what should we now do when gateway disconnects or for loss of OM connectivity?













MarketDataRequest
- note! MarketDataRequestReject if symbol doesn't exist at this time
- what is the expected response to unsubscribe?


```
--fix_debug=true
--metrics_listen_address 9413
--execution_report_using_quote_currency=true
--fix_heartbeat_freq=1000s
--exchange_time_as_sending_time=true
--log_path /opt/tbricks/roq-rencap/var/log/ftx-fix-real.log
--log_rotate_on_open=true
--log_max_files=10

--init_missing_md_entry_type_to_zero=true
--trade_summary_try_non_empty=true /opt/tbricks/roq-rencap/run/ftx-real.sock

--terminate_on_timeout=true

--cancel_on_disconnect=true
--silence_non_final_order_ack=true
--skip_order_download=true
--terminate_on_stream_disconnect=true

```
