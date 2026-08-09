#!/usr/bin/env bash
set -e

BUILD_BIN="./build/udp_chat"

if [ ! -f "$BUILD_BIN" ]; then
    echo "Executable $BUILD_BIN not found! Build it first."
    exit 1
fi

echo "--- Test 1: Start Client 1 (Alice) on port 9999 ---"
$BUILD_BIN Alice 9999 > client1.log 2>&1 &
C1_PID=$!
sleep 0.5

echo "--- Test 2: Start Client 2 (Bob) on port 9999 ---"
$BUILD_BIN Bob 9999 > client2.log 2>&1 &
C2_PID=$!
sleep 1.5

echo "--- Test 3: Terminate Client 2 with SIGINT ---"
kill -INT $C2_PID
wait $C2_PID 2>/dev/null || true
sleep 0.5

kill -INT $C1_PID 2>/dev/null || true
wait $C1_PID 2>/dev/null || true

if grep -q "joined the chat" client1.log && grep -q "left the chat" client1.log; then
    echo "PASSED: UDP broadcast join/leave messages received."
else
    echo "FAILED: Broadcast messages missing from log."
    cat client1.log
    rm -f client1.log client2.log
    exit 1
fi

rm -f client1.log client2.log
echo "ALL TESTS PASSED!"