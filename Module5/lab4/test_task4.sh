#!/bin/bash

MODULE="chardev_module"
KO_FILE="${MODULE}.ko"
DEV_FILE="/dev/my_chardev"
TEST_STRING="Chardev testing: Hello World!"

echo "=== Testing Task 4 (Character Device) ==="

if [ ! -f "$KO_FILE" ]; then
    echo "Error: $KO_FILE not found. Please run 'make' first."
    exit 1
fi

echo "1. Inserting module..."
sudo insmod "$KO_FILE"

echo "2. Verifying character device file exists..."
if [ -c "$DEV_FILE" ]; then
    echo "   [OK] $DEV_FILE exists and is a valid character device."
else
    echo "   [FAIL] $DEV_FILE does not exist or has an incorrect file type!"
    sudo rmmod "$MODULE" 2>/dev/null || true
    exit 1
fi

echo "3. Granting read/write permissions for testing..."
sudo chmod 666 "$DEV_FILE"

echo "4. Writing test string to $DEV_FILE..."
echo "$TEST_STRING" > "$DEV_FILE"
echo "   [OK] Data written."

echo "5. Reading back from $DEV_FILE..."
READ_DATA=$(cat "$DEV_FILE")
echo "   Read from device: \"$READ_DATA\""

if [ "$READ_DATA" = "$TEST_STRING" ]; then
    echo "   [OK] Read data matches written data exactly."
else
    echo "   [FAIL] Data mismatch! Read: \"$READ_DATA\", Expected: \"$TEST_STRING\""
    sudo rmmod "$MODULE"
    exit 1
fi

echo "6. Checking kernel log (dmesg) for chardev messages..."
if dmesg | tail -n 25 | grep -q "chardev:"; then
    echo "   [OK] Found device operations log in dmesg:"
    dmesg | tail -n 25 | grep "chardev:"
else
    echo "   [WARNING] No logs found in dmesg."
fi

echo "7. Removing module..."
sudo rmmod "$MODULE"

echo "8. Verifying device file is removed..."
if [ ! -e "$DEV_FILE" ]; then
    echo "   [OK] $DEV_FILE successfully removed."
else
    echo "   [FAIL] $DEV_FILE still exists after rmmod!"
    exit 1
fi

echo "=== Task 4 Test Completed Successfully ==="