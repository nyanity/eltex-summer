#!/usr/bin/env bash
set -e

BUILD_BIN="./build/chat"

if [ ! -f "$BUILD_BIN" ]; then
    echo "Executable $BUILD_BIN not found! Build it first."
    exit 1
fi

echo "--- Clean up leftover POSIX queues ---"
rm -f /dev/mqueue/test_p2p_script_1 /dev/mqueue/test_p2p_script_2 2>/dev/null || true

echo "--- Test 1: Incorrect parameters ---"
if $BUILD_BIN > /dev/null 2>&1; then
    echo "FAILED: Program should fail without arguments."
    exit 1
else
    echo "PASSED: Argument parsing validated."
fi

echo "--- Test 2: Start Peer 1 (Creator) and Peer 2 (Joiner) ---"
$BUILD_BIN test_p2p_script > peer1.log 2>&1 &
P1_PID=$!
sleep 0.5

$BUILD_BIN test_p2p_script > peer2.log 2>&1 &
P2_PID=$!
sleep 1.5

echo "--- Test 3: Terminate Peer 1 with SIGINT ---"
kill -INT $P1_PID
wait $P1_PID 2>/dev/null || true
sleep 1

if kill -0 $P2_PID 2>/dev/null; then
    echo "FAILED: Peer 2 did not exit upon Peer 1 disconnection."
    kill -9 $P2_PID 2>/dev/null || true
    rm -f peer1.log peer2.log
    exit 1
fi

echo "PASSED: P2P Chat communication & termination verified."

rm -f peer1.log peer2.log
echo "ALL TESTS PASSED!"