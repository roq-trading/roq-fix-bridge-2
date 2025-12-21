.. |checkmark| unicode:: U+2713
.. |rightarrow| unicode:: U+2192
.. |rightdoublearrow| unicode:: U+21D2
.. |righttriplearrow| unicode:: U+21DB

.. _roq-fix-bridge:

roq-fix-bridge
==============


Links
-----

* `FIX Trading Community <https://www.fixtrading.org/>`_


Purpose
-------

* Gateway bridge using a FIX 4.4 based message protocol
* Integration with third-party vendors


Description
-----------

The FIX bridge allows FIX clients to connect with gateways, receive market
data, and manage orders.

The FIX bridge configuration defines a list of exchanges and corresponding
symbols (regex).
This implementation use :code:`exchange`/:code:`symbol` for *all* message
routing!

.. NOTE::
   Some FIX clients may rely on :code:`TradingSessionID` for state management.
   For many Crypto exchanges, this is not a well-defined concept.
   The implementation choice is to use the :code:`source_name` of the gateway
   (as specified using the gateway :code:`--name` command-line flag) to mean
   :code:`TradingSessionID`.
   The concept of :code:`TradingSessionID` is otherwise not used directly by
   the FIX bridge for any message routing.

The FIX bridge will automatically manage gateway connectivity.

Upon connecting to a gateway, the FIX bridge will discover the universe of
:code:`exchange`/:code:`symbol` as well as order management capabilities, e.g.
whether order rewrite is supported by a gateway.
Furthermore, a "ready"-state is being maintained based on gateway notifications
around account availability.

.. NOTE::
   FIX clients should expect and manager order action rejects if the
   :code:`exchange`/:code:`symbol` has not yet been discovered and/or the
   account is not yet in the ready state (e.g. due to disconnect).

Order management has been implemented to conform to
`FIX Order State Changes <https://www.fixtrading.org/online-specification/order-state-changes/>`__.

This effectively means that :code:`ClOrdId`/:code:`OrigClOrdId` chaining is
supported when modifying and canceling orders.

When possible, gateways will try to pass enough information to the exchange
such that later order download will be able to notify the FIX bridge with the
correct :code:`ClOrdId`/:code:`OrigClOrdId` information.
This is however only *best effort* and the feature should *not* be relied upon!

.. NOTE::
   The FIX bridge and the gateways are all designed for low latency and there is
   no database persistence layer causing latency anywhere in the path of order
   routing.
   Furthermore, not all exchanges support rewriting client order identifiers.
   The FIX bridge and the gateways maintain in-memory maps between exchange
   order identifiers and outstanding requests on the one side, and
   :code:`ClOrdId`/:code:`OrigClOrdId` on the other side.
   Correct updates around :code:`ClOrdId`/:code:`OrigClOrdId` chaining should
   only be expected if both FIX bridge and gateway remain connected.
   Reconnection *may* work, but it's best effort, only.

To manage uncertainties around disconnects, the FIX bridge may be configured
such that gateways are instructed to auto-cancel orders upon disconnect.
Similarly, when possible, gateways may be configured such that exchanges are
instructed to auto-cancel orders upon disconnect.


Conda
-----

* :ref:`Using Conda <tutorial-conda>`

.. tab:: Install
  
  .. code-block:: bash
  
    $ conda install \
      --channel https://roq-trading.com/conda/stable \
      roq-fix-bridge
  
.. tab:: Configure

  .. code-block:: bash
  
    $ cp --config_file $CONDA_PREFIX/share/roq-fix-bridge/config.toml $CONFIG_FILE_PATH
  
    # Then modify $CONFIG_FILE_PATH to match your specific configuration
  
.. tab:: Run
  
  .. code-block:: bash
  
    $ roq-fix-bridge \
          --name "fix-bridge" \
          --config_file "$CONFIG_FILE_PATH" \
          --service_listen_address "$TCP_LISTEN_PORT_FOR_METRICS" \
          --client_listen_address "$TCP_LISTEN_PORT_FOR_FIX_CLIENTS" \
          --flagfile "$FLAG_FILE" \
          [<gateway unix domain sockets>]
  

Config
------

.. tab:: Symbols

  A list of exchanges and corresponding symbols.
  
  .. code-block:: toml

    [symbols]
   
     [symbols.bitmex]
     regex=["XBTUSD", "XBT.*"]
   
     [symbols.deribit]
     regex=".*"
   
.. tab:: Users
  
  A list of FIX clients allowed to connect to the FIX bridge.

  .. code-block:: toml

    [users]
   
     [users.MD1]
     component="roq-fix-client-test"
     username="tbmd1"

.. tab:: Statistics

  An optional list of mapping overrides.
  This may be necessary because FIX 4.4 has a limited set of market data types (MDEntryType).
  
  .. code-block:: toml

    [statistics]
   
      [statistics.FUNDING_RATE]
      fix_md_entry_type="SETTLEMENT_PRICE"
   
.. tab:: Errors

  An optional list of mapping overrides.
  This may be necessary because FIX 4.4 has a limited set of cancel reject reasons (CxlRejReason).
  
  .. code-block:: toml

    [errors]
   
      [errors.REQUEST_RATE_LIMIT_REACHED]
      cxl_rej_reason=1139
   

Flags
-----

* :ref:`Using Flags <abseil-cpp>`

.. code-block:: bash

   $ roq-fix-bridge --help

.. tab:: Common

   .. include:: flags/common.rstinc

.. tab:: OMS

   .. include:: flags/oms.rstinc

.. tab:: FIX

   .. include:: flags/fix.rstinc

.. tab:: Messaging

   .. include:: flags/messaging.rstinc

.. tab:: Test

   .. include:: flags/test.rstinc


Session
-------

Client |rightarrow| Server
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: inbound/logon.rstinc

.. include:: inbound/logout.rstinc

.. include:: inbound/test_request.rstinc

.. include:: inbound/heartbeat.rstinc

.. include:: inbound/reject.rstinc

Server |rightarrow| Client
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: outbound/logon.rstinc

.. include:: outbound/logout.rstinc

.. include:: outbound/test_request.rstinc

.. include:: outbound/heartbeat.rstinc

.. include:: outbound/reject.rstinc


Market Data
-----------

Client |rightarrow| Server
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: inbound/security_list_request.rstinc

.. include:: inbound/security_definition_request.rstinc

.. include:: inbound/security_status_request.rstinc

.. include:: inbound/market_data_request.rstinc

.. include:: inbound/trading_session_status_request.rstinc

Server |rightarrow| Client
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: outbound/security_list.rstinc

.. include:: outbound/security_definition.rstinc

.. include:: outbound/security_status.rstinc

.. include:: outbound/market_data_snapshot_full_refresh.rstinc

.. include:: outbound/market_data_incremental_refresh.rstinc

.. include:: outbound/market_data_request_reject.rstinc

.. include:: outbound/trading_session_status.rstinc


Order Management
----------------

Client |rightarrow| Server
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: inbound/order_status_request.rstinc

.. include:: inbound/order_mass_status_request.rstinc

.. include:: inbound/new_order_single.rstinc

.. include:: inbound/order_cancel_request.rstinc

.. include:: inbound/order_cancel_replace_request.rstinc

.. include:: inbound/order_mass_cancel_request.rstinc

Server |rightarrow| Client
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: outbound/execution_report.rstinc

.. include:: outbound/order_cancel_reject.rstinc

.. include:: outbound/order_mass_cancel_report.rstinc


Trade Reporting
---------------

Client |rightarrow| Server
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: inbound/trade_capture_report_request.rstinc

Server |rightarrow| Client
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: outbound/trade_capture_report.rstinc

.. include:: outbound/trade_capture_report_request_ack.rstinc

Position Management
-------------------

Client |rightarrow| Server
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: inbound/request_for_positions.rstinc

Server |rightarrow| Client
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: outbound/position_report.rstinc

Miscellaneous
-------------

Server |rightarrow| Client
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: outbound/business_message_reject.rstinc


.. _roq-fix-bridge-authentication:

Authentication
--------------

The simplest version is plain comparison on the password string.

:code:`hmac_sha256`
~~~~~~~~~~~~~~~~~~~

The connecting client must compute a nonce and pass this as :code:`Logon.raw_data`.

A signature is the base64 encoding of the HMAC/SHA256 digest (using a shared secret).
The connecting client must pass this as :code:`Logon.password`.

:code:`hmac_sha256_ts`
~~~~~~~~~~~~~~~~~~~~~~

This is the same algorithm as :code:`hmac_sha256` with the only difference being
a millisecond timestamp and a period (:code:`.`) being prepended to the nonce.

The server side can then extract the timestamp and validate against its own clock.


.. _roq-fix-bridge-request_id:

Request ID's
------------

* Must conform to the character set from web-safe base 64 encoding (:code:`a-zA-Z0-9-_{1,24}`).

.. _roq-fix-bridge-routing:

Routing
-------

The bridge can operate in one of two modes.

Default is routing by account.

Routing by strategy can be enabled by using the :code:`--oms_route_by_strategy` flag.
Party information is then required and expected to identify the strategy.


.. _roq-fix-bridge-params:

Parameters
----------

It is possible to use the text field to pass additional parameters.

For example, the :code:`NewOrderSingle` request may optionally accept a
specific :code:`margin_mode`.

These parameters are key value pairs,
each pair using the assignment operator (:code:`=`) to separate key and value,
pairs separated by semi-colon (code:`;`).

Enums
-----

The following defines the maximum set of values allowed.
Exchanges / gateways may impose further restrictions.

Side <54>
~~~~~~~~~

* :code:`1` (:code:`BUY`)
* :code:`2` (:code:`SELL`)

TimeInForce <59>
~~~~~~~~~~~~~~~~

* :code:`0` (:code:`GFD`)
* :code:`1` (:code:`GTC`)
* :code:`2` (:code:`OPG`)
* :code:`3` (:code:`IOC`)
* :code:`4` (:code:`FOK`)
* :code:`5` (:code:`GTX`)
* :code:`6` (:code:`GTD`)
* :code:`7` (:code:`AT_THE_CLOSE`)
* :code:`8` (:code:`GOOD_THROUGH_CROSSING`)
* :code:`9` (:code:`AT_CROSSING`)
* :code:`A` (:code:`GOOD_FOR_TIME`)

PositionEffect <77>
~~~~~~~~~~~~~~~~~~~

* :code:`C` (:code:`CLOSE`)
* :code:`O` (:code:`OPEN`)

ExecInst <18>
~~~~~~~~~~~~~

* :code:`6` (:code:`PARTICIPATE_DO_NOT_INITIATE`)
* :code:`E` (:code:`DO_NOT_INCREASE`)
* :code:`F` (:code:`DO_NOT_REDUCE`)
* :code:`Z` (:code:`CANCEL_IF_NOT_BEST`)

Constraints
-----------

* The bridge must register as a regular user to the gateways.
  The implication is that the bridge only has visibility to that user, i.e.
  there is no option to see the orders managed by other users.
  Thus, there is currently no option to implement a true drop-copy solution.
* The bridge requires each downstream client to be mapped 1:1 with an account.
  This is currently the only option to route order acks, order updates and
  trades back to the correct client.
  Note! The C++ API does support account multiplexing.
* The universe of {exchange, symbol} is dynamic and can not always be known at the time
  when a FIX client issues a request.
  The FIX protocol also does not allow for a client to receive notifications when an exchange
  hase become ready.
  Thus, a FIX client must **expect rejects** (when an {exchange, symbol} combination has not yet
  become available, for example) and implement a **retry policy**.
* Successful FIX client subscriptions will be cached and survive gateway reconnects.
  This is done because the FIX protocol does not allow for a client to receive notifications
  when a subscription is broken.
  Thus, a FIX client must **expect stale subscriptions** and implement a **timeout policy**.
* The FIX protocol requires subsequent order actions to be rejected if a previous request
  has been rejected.
  Full support is only possible if the exchanges support it or if the gateways disallow
  pipelining (sending multiple requests without first waiting for the previous request's
  response).
  A low-latency configuration should allow pipelining (at the gateway level) and the FIX
  bridge can therefore not be fully FIX compliant (for the reasons just described).
* Custom order book calculations are supported but constrained to one custom calculation per
  market data request.
  It is not possible to overlap custom calculations with regular updates (like top of book
  or market by price) due to the FIX protocol allowing for any other special identifiers with the
  full/incremental updates.

Comments
--------

* The bridge can operate in one of two modes:

  * Routing by :code:`account` (the default).
    In this mode a client connection must map 1:1 with an account.
    All order updates will be routed based on the account name.

  * Routing by :code:`strategy_id` (opt-in, used by :code:`roq-fix-proxy`).
    In this mode a client connection must dynamically register routes based on
    :code:`strategy_id`'s.
    The route is automatically removed upon seeing the client disconnecting.
    All order updates will be routed based on the strategy.

* It is possible to use :code:`NewOrderSingle.text` to map special fields, e.g.

  * :code:`margin_mode=isolated`
  * :code:`margin_mode=cross`

  .. note::
     This is currently an **EXPERIMENTAL** feature meant to work around FIX protocol limitations.
     Choices around the format (key-value pairs, separator, naming conventions, etc.) may still be changed.
