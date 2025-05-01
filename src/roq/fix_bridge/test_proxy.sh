#!/usr/bin/env bash

NAME="trader"

CONFIG_FILE="config/$NAME-proxy.toml"

CWD="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

# debug?

if [ "$1" == "debug" ]; then
  KERNEL="$(uname -a)"
  case "$KERNEL" in
    Linux*)
      PREFIX="gdb --args"
      ;;
    Darwin*)
      PREFIX="lldb --"
      ;;
  esac
  shift 1
else
	PREFIX=
fi

# launch

$PREFIX "./roq-fix-bridge" \
	--name "$NAME" \
	--config_file "$CONFIG_FILE" \
  --service_listen_address 1234 \
  --client_listen_address $HOME/run/fix-bridge.sock \
  --init_missing_md_entry_type_to_zero=true \
  --oms_route_by_strategy=true \
	$@
