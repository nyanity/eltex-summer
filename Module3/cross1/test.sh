#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SERVER_BIN="$SCRIPT_DIR/build/chat_server"
CLIENT_BIN="$SCRIPT_DIR/build/chat_client"

if [ ! -f "$SERVER_BIN" ] || [ ! -f "$CLIENT_BIN" ]; then
    echo "Executables not found! Build them first."
    exit 1
fi

if [ "$(id -u)" -ne 0 ]; then
    echo "SKIPPED: Root (sudo) privileges are required to run RAW echo tests."
    exit 0
fi

echo "--- Clean up stale processes ---"
pkill -f chat_server 2>/dev/null || true
pkill -f chat_client 2>/dev/null || true
rm -f "$SCRIPT_DIR/server.log" "$SCRIPT_DIR/client1.log" "$SCRIPT_DIR/client2.log" 2>/dev/null || true
sleep 0.2

echo "--- Test 1: Start RAW Echo Server ---"
"$SERVER_BIN" 9999 > "$SCRIPT_DIR/server.log" 2>&1 &
SERVER_PID=$!
sleep 0.5

if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "FAILED: Server failed to start."
    cat "$SCRIPT_DIR/server.log"
    exit 1
fi

echo "--- Test 2: Start Client 1 ---"
(
  echo "Hello"
  sleep 0.4
  echo "World"
  sleep 0.4
) | "$CLIENT_BIN" 8888 127.0.0.1 9999 > "$SCRIPT_DIR/client1.log" 2>&1 &
C1_PID=$!
sleep 0.5

echo "--- Test 3: Start Client 2 ---"
(
  echo "Ping"
  sleep 0.4
) | "$CLIENT_BIN" 8887 127.0.0.1 9999 > "$SCRIPT_DIR/client2.log" 2>&1 &
C2_PID=$!
sleep 1.0

kill -TERM $C1_PID 2>/dev/null || true
kill -TERM $C2_PID 2>/dev/null || true
kill -TERM $SERVER_PID 2>/dev/null || true

wait $SERVER_PID 2>/dev/null || true

echo "--- Test 4: Verify Server Counters ---"
if grep -q "Hello 1" "$SCRIPT_DIR/client1.log" && grep -q "World 2" "$SCRIPT_DIR/client1.log" && grep -q "Ping 1" "$SCRIPT_DIR/client2.log"; then
    echo "PASSED: Raw echo-server counted client sessions correctly!"
else
    echo "FAILED: Incorrect echo server counting."
    echo "Client 1 Log:"
    cat "$SCRIPT_DIR/client1.log"
    echo "Client 2 Log:"
    cat "$SCRIPT_DIR/client2.log"
    exit 1
fi

rm -f "$SCRIPT_DIR/server.log" "$SCRIPT_DIR/client1.log" "$SCRIPT_DIR/client2.log"
echo "ALL TESTS PASSED!"