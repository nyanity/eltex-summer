#!/usr/bin/env bash
set -e

SERVER_BIN="./build/chat_server"
CLIENT_BIN="./build/chat_client"

if [ ! -f "$SERVER_BIN" ] || [ ! -f "$CLIENT_BIN" ]; then
    echo "Executables not found! Build them first."
    exit 1
fi

echo "--- Clean up any stale server or client processes ---"
pkill -f chat_server 2>/dev/null || true
pkill -f chat_client 2>/dev/null || true
rm -f test_send.txt test_send.txt.received 2>/dev/null || true
sleep 0.2

echo "--- Create test payload file ---"
echo "This is the file transfer payload content. Let's verify TCP delivery!" > test_send.txt

echo "--- Test 1: Start TCP Chat Server ---"
$SERVER_BIN 9999 > server.log 2>&1 &
SERVER_PID=$!
sleep 0.5

if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "FAILED: Server failed to start."
    cat server.log
    exit 1
fi

echo "--- Test 2: Start Client Bob (Receiver) ---"
$CLIENT_BIN Bob 127.0.0.1 9999 > client2.log 2>&1 &
C2_PID=$!
sleep 0.2

echo "--- Test 3: Start Client Alice (Sender) with File Command ---"
(
  echo "Hello from Alice"
  sleep 0.4
  echo "/file test_send.txt"
  sleep 0.5
) | $CLIENT_BIN Alice 127.0.0.1 9999 > client1.log 2>&1 &
C1_PID=$!

sleep 2.0

kill -TERM $C1_PID 2>/dev/null || true
kill -TERM $C2_PID 2>/dev/null || true
kill -TERM $SERVER_PID 2>/dev/null || true

wait $SERVER_PID 2>/dev/null || true

echo "--- Test 4: Verify File Delivery ---"
if [ ! -f "test_send.txt.received" ]; then
    echo "FAILED: Received file 'test_send.txt.received' not found!"
    cat client2.log
    rm -f test_send.txt test_send.txt.received server.log client1.log client2.log
    exit 1
fi

if cmp -s test_send.txt test_send.txt.received; then
    echo "PASSED: File sent and received identically."
else
    echo "FAILED: Received file content does not match the original!"
    rm -f test_send.txt test_send.txt.received server.log client1.log client2.log
    exit 1
fi

rm -f test_send.txt test_send.txt.received server.log client1.log client2.log
echo "ALL TESTS PASSED!"