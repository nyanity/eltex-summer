#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_BIN="$SCRIPT_DIR/build/taxi"

if [ ! -f "$BUILD_BIN" ]; then
    echo "Executable $BUILD_BIN not found! Build it first."
    exit 1
fi

echo "--- Clean up stale processes ---"
pkill -f build/taxi 2>/dev/null || true
rm -f "$SCRIPT_DIR/cli_in.pipe" "$SCRIPT_DIR/taxi.log" 2>/dev/null || true
sleep 0.2

echo "--- Create FIFO pipe for dynamic stdin ---"
mkfifo "$SCRIPT_DIR/cli_in.pipe"

echo "--- Test 1: Start Taxi Control Center ---"
"$BUILD_BIN" < "$SCRIPT_DIR/cli_in.pipe" > "$SCRIPT_DIR/taxi.log" 2>&1 &
TAXI_PID=$!
sleep 0.5

if ! kill -0 $TAXI_PID 2>/dev/null; then
    echo "FAILED: Taxi Control Center failed to start."
    cat "$SCRIPT_DIR/taxi.log"
    rm -f "$SCRIPT_DIR/cli_in.pipe"
    exit 1
fi

echo "--- Test 2: Spawn dynamic Driver process ---"
exec 3> "$SCRIPT_DIR/cli_in.pipe"
echo "create_driver" >&3
sleep 0.8

DRIVER_PID=$(grep -o -E "Driver [0-9]+ connected" "$SCRIPT_DIR/taxi.log" | head -n 1 | awk '{print $2}')
if [ -z "$DRIVER_PID" ]; then
    echo "FAILED: Driver was not spawned or connected correctly."
    cat "$SCRIPT_DIR/taxi.log"
    exec 3>&-
    kill -KILL $TAXI_PID 2>/dev/null || true
    rm -f "$SCRIPT_DIR/cli_in.pipe" "$SCRIPT_DIR/taxi.log"
    exit 1
fi
echo "PASSED: Extracted driver PID: $DRIVER_PID"

echo "--- Test 3: Send Task to Driver ---"
echo "send_task $DRIVER_PID 2" >&3
sleep 0.3

echo "--- Test 4: Verify Busy Status ---"
echo "get_status $DRIVER_PID" >&3
sleep 0.3

echo "--- Test 5: Wait for timer expiration ---"
sleep 2.5

echo "--- Test 6: Verify Available Status ---"
echo "get_status $DRIVER_PID" >&3
sleep 0.3

exec 3>&-
sleep 0.5

kill -TERM $TAXI_PID 2>/dev/null || true
wait $TAXI_PID 2>/dev/null || true

echo "--- Test 7: Verify Log Output ---"
if grep -q "Busy 2" "$SCRIPT_DIR/taxi.log" && grep -q "completed task and is now Available" "$SCRIPT_DIR/taxi.log"; then
    echo "PASSED: State machine, asynchronous timers, and epoll notifications verified!"
    echo "Console Output:"
    cat "$SCRIPT_DIR/taxi.log"
else
    echo "FAILED: Log does not contain expected state transitions."
    echo "Console Output:"
    cat "$SCRIPT_DIR/taxi.log"
    rm -f "$SCRIPT_DIR/cli_in.pipe" "$SCRIPT_DIR/taxi.log"
    exit 1
fi

rm -f "$SCRIPT_DIR/cli_in.pipe" "$SCRIPT_DIR/taxi.log"
echo "ALL TESTS PASSED!"