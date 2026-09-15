#!/bin/bash

MODULE="proc_module"
KO_FILE="${MODULE}.ko"
PROC_FILE="/proc/my_proc_file"
TEST_STRING="Hello, Kernel! This is a test string."

echo "=== Testing Task 2 (Proc File) ==="

if [ ! -f "$KO_FILE" ]; then
    echo "Error: $KO_FILE not found. Please run 'make' first."
    exit 1
fi

echo "1. Inserting module..."
sudo insmod "$KO_FILE"

echo "2. Verifying proc file exists..."
if [ -f "$PROC_FILE" ]; then
    echo "   [OK] $PROC_FILE exists."
else
    echo "   [FAIL] $PROC_FILE does not exist!"
    sudo rmmod "$MODULE" 2>/dev/null || true
    exit 1
fi

echo "3. Writing test string to $PROC_FILE..."
echo "$TEST_STRING" | sudo tee "$PROC_FILE" > /dev/null

echo "4. Reading back from $PROC_FILE..."
READ_DATA=$(cat "$PROC_FILE")
echo "   Read from file: \"$READ_DATA\""

if [ "$READ_DATA" = "$TEST_STRING" ]; then
    echo "   [OK] Read data matches written data exactly."
else
    echo "   [FAIL] Data mismatch! Read: \"$READ_DATA\", Expected: \"$TEST_STRING\""
    sudo rmmod "$MODULE"
    exit 1
fi

echo "5. Checking kernel log (dmesg) for received message..."
if dmesg | tail -n 20 | grep -q "Received from userspace"; then
    echo "   [OK] Found confirmation in dmesg:"
    dmesg | tail -n 20 | grep "Received from userspace"
else
    echo "   [WARNING] Confirmation not found in dmesg."
fi

echo "6. Removing module..."
sudo rmmod "$MODULE"

echo "7. Verifying proc file is removed..."
if [ ! -f "$PROC_FILE" ]; then
    echo "   [OK] $PROC_FILE successfully removed."
else
    echo "   [FAIL] $PROC_FILE still exists after rmmod!"
    exit 1
fi

echo "=== Task 2 Test Completed Successfully ==="