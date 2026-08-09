#!/usr/bin/env bash
set -e

BUILD_BIN="./build/shm_prod_cons"

if [ ! -f "$BUILD_BIN" ]; then
    echo "Executable $BUILD_BIN not found! Build it first."
    exit 1
fi

echo "--- Clean up leftover SHM/SEM if any ---"
KEY_HEX=$(printf "0x%x" $(python3 -c "import os; print(os.stat('.').st_ino & 0xffff)" 2>/dev/null || echo 0))
ipcrm -M 0x53484d34 2>/dev/null || true
ipcrm -S 0x53484d34 2>/dev/null || true

echo "--- Test 1: Start Producer in background ---"
$BUILD_BIN -p > producer.log 2>&1 &
PROD_PID=$!
sleep 0.2

if ! kill -0 $PROD_PID 2>/dev/null; then
    echo "FAILED: Producer failed to start."
    cat producer.log
    exit 1
fi

echo "--- Test 2: Start 2 Consumers in background ---"
$BUILD_BIN -c > consumer1.log 2>&1 &
C1_PID=$!

$BUILD_BIN -c > consumer2.log 2>&1 &
C2_PID=$!

echo "--- Waiting for Producer and Consumers to complete ---"
wait $PROD_PID
wait $C1_PID
wait $C2_PID

echo "PASSED: All processes completed successfully."

rm -f producer.log consumer1.log consumer2.log
echo "ALL TESTS PASSED!"