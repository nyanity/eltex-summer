#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_BIN="$SCRIPT_DIR/build/raw_capture"
CHAT_DIR="$(cd "$SCRIPT_DIR/../06" && pwd)"
CHAT_BIN="$CHAT_DIR/build/udp_chat"

if [ ! -f "$BUILD_BIN" ]; then
    echo "Executable $BUILD_BIN not found! Build it first."
    exit 1
fi

echo "--- Checking Task 6 Chat Executable ---"
if [ ! -f "$CHAT_BIN" ]; then
    echo "Task 6 executable not found. Compiling it..."
    if [ -d "$CHAT_DIR" ]; then
        make -C "$CHAT_DIR"
    else
        echo "Error: Directory $CHAT_DIR not found. Cannot compile Task 6 chat."
        exit 1
    fi
fi

if [ "$(id -u)" -ne 0 ]; then
    echo "SKIPPED: Root (sudo) privileges are required to run RAW packet capture tests."
    exit 0
fi

echo "--- Clean up stale processes ---"
pkill -f udp_chat 2>/dev/null || true
pkill -f raw_capture 2>/dev/null || true
rm -f "$SCRIPT_DIR/capture.log" "$SCRIPT_DIR/dns_capture.log" 2>/dev/null || true
sleep 0.2

echo "--- Test 1: Start RAW Capture for Chat on port 8889 ---"
"$BUILD_BIN" -c 8889 1 > "$SCRIPT_DIR/capture.log" 2>&1 &
CAP_PID=$!
sleep 0.5

echo "--- Test 2: Generate Chat traffic on port 8889 ---"
echo "RAW_CAPTURE_PAYLOAD" | "$CHAT_BIN" TestUser 8889 > /dev/null 2>&1 &
CHAT_PID=$!

sleep 1.5

kill -TERM $CHAT_PID 2>/dev/null || true
kill -TERM $CAP_PID 2>/dev/null || true

if grep -q "JOIN:" "$SCRIPT_DIR/capture.log"; then
    echo "PASSED: Chat packet capture verified!"
    echo "Captured output:"
    cat "$SCRIPT_DIR/capture.log"
else
    echo "FAILED: Expected chat message not captured."
    cat "$SCRIPT_DIR/capture.log"
    rm -f "$SCRIPT_DIR/capture.log"
    exit 1
fi

echo "--- Test 3: Start RAW Capture for DNS on port 53 ---"
"$BUILD_BIN" -d 53 1 > "$SCRIPT_DIR/dns_capture.log" 2>&1 &
DNS_CAP_PID=$!
sleep 0.5

echo "--- Test 4: Generate DNS query traffic ---"
if command -v nslookup >/dev/null 2>&1; then
    nslookup example.com 8.8.8.8 >/dev/null 2>&1 || true
elif command -v host >/dev/null 2>&1; then
    host example.com 8.8.8.8 >/dev/null 2>&1 || true
else
    echo -e -n "\x00\x00\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00\x07example\x03com\x00\x00\x01\x00\x01" > /dev/udp/8.8.8.8/53 2>/dev/null || true
fi

sleep 1.5
kill -TERM $DNS_CAP_PID 2>/dev/null || true

if [ -s "$SCRIPT_DIR/dns_capture.log" ]; then
    echo "PASSED: DNS packet capture verified!"
    echo "Captured output:"
    cat "$SCRIPT_DIR/dns_capture.log"
else
    echo "FAILED: DNS query not captured."
    cat "$SCRIPT_DIR/dns_capture.log"
    rm -f "$SCRIPT_DIR/capture.log" "$SCRIPT_DIR/dns_capture.log"
    exit 1
fi

rm -f "$SCRIPT_DIR/capture.log" "$SCRIPT_DIR/dns_capture.log"
echo "ALL TESTS PASSED!"