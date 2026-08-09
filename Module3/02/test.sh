#!/usr/bin/env bash
set -e

BUILD_BIN="./build/pubsub"

if [ ! -f "$BUILD_BIN" ]; then
    echo "Executable $BUILD_BIN not found! Build it first."
    exit 1
fi

echo "--- Cleaning up old message queue if exists ---"
KEY_HEX=$(printf "0x%x" $(python3 -c "import os; print(os.stat('.').st_ino & 0xffff)" 2>/dev/null || echo 0))
ipcrm -Q 0x4d534751 2>/dev/null || true

echo "--- Test 1: Start Broker ---"
$BUILD_BIN -b > broker.log 2>&1 &
BROKER_PID=$!
sleep 0.5

if ! kill -0 $BROKER_PID 2>/dev/null; then
    echo "FAILED: Broker did not start properly."
    cat broker.log
    exit 1
fi
echo "PASSED: Broker running."

echo "--- Test 2: Duplicate Broker start ---"
if $BUILD_BIN -b > /dev/null 2>&1; then
    echo "FAILED: Duplicate broker should have failed."
    kill -9 $BROKER_PID 2>/dev/null || true
    exit 1
else
    echo "PASSED: Duplicate broker rejected as expected."
fi

echo "--- Test 3: Start Subscriber & Publisher ---"
$BUILD_BIN -s sports > subscriber.log 2>&1 &
SUB_PID=$!
sleep 0.5

$BUILD_BIN -p sports > publisher.log 2>&1 &
PUB_PID=$!
sleep 1

echo "--- Test 4: Terminate Broker and check signal propagation ---"
kill -INT $BROKER_PID
wait $BROKER_PID 2>/dev/null || true
sleep 0.5

if kill -0 $SUB_PID 2>/dev/null; then
    echo "FAILED: Subscriber was not terminated by SIGINT."
    kill -9 $SUB_PID 2>/dev/null || true
    rm -f broker.log subscriber.log publisher.log
    exit 1
fi

if kill -0 $PUB_PID 2>/dev/null; then
    echo "FAILED: Publisher was not terminated by SIGINT."
    kill -9 $PUB_PID 2>/dev/null || true
    rm -f broker.log subscriber.log publisher.log
    exit 1
fi

echo "PASSED: All processes terminated on broker shutdown."

rm -f broker.log subscriber.log publisher.log
echo "ALL TESTS PASSED!"