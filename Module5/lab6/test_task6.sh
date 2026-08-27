#!/bin/bash
# Test script for Task 6 (IP Blacklist Module using Netfilter)

MODULE="ip_filter_module"
KO_FILE="${MODULE}.ko"
PROC_FILE="/proc/ip_blacklist"
TEST_IP="8.8.8.8"

echo "=== Testing Task 6 (Netfilter IP Blacklist) ==="

if [ ! -f "$KO_FILE" ]; then
    echo "Error: $KO_FILE not found. Please run 'make' first."
    exit 1
fi

echo "1. Inserting module..."
sudo insmod "$KO_FILE"

echo "2. Verifying proc entry exists..."
if [ -f "$PROC_FILE" ]; then
    echo "   [OK] $PROC_FILE exists."
else
    echo "   [FAIL] $PROC_FILE does not exist!"
    sudo rmmod "$MODULE" 2>/dev/null || true
    exit 1
fi

echo "3. Adding $TEST_IP to blacklist..."
echo "+$TEST_IP" | sudo tee "$PROC_FILE" > /dev/null

echo "4. Reading blacklist..."
BLACKLIST_CONTENT=$(cat "$PROC_FILE")
echo "   Blacklist contains:"
echo "   $BLACKLIST_CONTENT"

if echo "$BLACKLIST_CONTENT" | grep -q "$TEST_IP"; then
    echo "   [OK] IP successfully blacklisted."
else
    echo "   [FAIL] IP not found in blacklist!"
    sudo rmmod "$MODULE"
    exit 1
fi

echo "5. Testing outgoing request block (sending 1 ping to $TEST_IP)..."
# Получаем текущее количество строк в логах dmesg для точной фильтрации
INITIAL_LOG_COUNT=$(dmesg | wc -l)

# Отправляем тестовый пинг (он должен завершиться ошибкой или потерей пакета)
ping -c 1 -W 2 "$TEST_IP" > /dev/null 2>&1 || true

echo "6. Checking kernel logs (dmesg) for drop confirmation..."
# Считываем только новые строки, появившиеся после начала теста
NEW_LOGS=$(dmesg | tail -n +$((INITIAL_LOG_COUNT + 1)))

if echo "$NEW_LOGS" | grep -q "BLOCKED outgoing packet to $TEST_IP"; then
    echo "   [OK] Netfilter successfully dropped the packet!"
    echo "$NEW_LOGS" | grep "BLOCKED outgoing packet to $TEST_IP"
else
    echo "   [FAIL] Netfilter did not report dropping the packet!"
    sudo rmmod "$MODULE"
    exit 1
fi

echo "7. Removing $TEST_IP from blacklist..."
echo "-$TEST_IP" | sudo tee "$PROC_FILE" > /dev/null

echo "8. Verifying $TEST_IP is removed from blacklist..."
BLACKLIST_CONTENT=$(cat "$PROC_FILE")
if ! echo "$BLACKLIST_CONTENT" | grep -q "$TEST_IP"; then
    echo "   [OK] IP successfully removed from blacklist."
else
    echo "   [FAIL] IP still in blacklist!"
    sudo rmmod "$MODULE"
    exit 1
fi

echo "9. Clearing the entire blacklist..."
echo "clear" | sudo tee "$PROC_FILE" > /dev/null
echo "   [OK] Cleared."

echo "10. Removing module..."
sudo rmmod "$MODULE"

echo "11. Verifying proc entry is removed..."
if [ ! -f "$PROC_FILE" ]; then
    echo "   [OK] $PROC_FILE successfully removed."
else
    echo "   [FAIL] $PROC_FILE still exists after rmmod!"
    exit 1
fi

echo "=== Task 6 Test Completed Successfully ==="