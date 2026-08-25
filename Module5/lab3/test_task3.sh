#!/bin/bash

MODULE="kbleds_sysfs"
KO_FILE="${MODULE}.ko"
SYSFS_FILE="/sys/kernel/kbleds/mask"

echo "=== Testing Task 3 (Keyboard LEDs via Sysfs) ==="

if [ ! -f "$KO_FILE" ]; then
    echo "Error: $KO_FILE not found. Please run 'make' first."
    exit 1
fi

echo "1. Inserting module..."
sudo insmod "$KO_FILE"

echo "2. Verifying sysfs file exists..."
if [ -f "$SYSFS_FILE" ]; then
    echo "   [OK] $SYSFS_FILE exists."
else
    echo "   [FAIL] $SYSFS_FILE does not exist!"
    sudo rmmod "$MODULE" 2>/dev/null || true
    exit 1
fi

echo "3. Testing writing valid mask '7' (all LEDs)..."
if echo 7 | sudo tee "$SYSFS_FILE" > /dev/null; then
    echo "   [OK] Wrote '7' successfully."
else
    echo "   [FAIL] Could not write to $SYSFS_FILE"
    sudo rmmod "$MODULE"
    exit 1
fi

echo "4. Reading back mask from sysfs..."
READ_MASK=$(cat "$SYSFS_FILE")
echo "   Read mask: \"$READ_MASK\""
if [ "$READ_MASK" = "7" ]; then
    echo "   [OK] Mask is correctly set to 7."
else
    echo "   [FAIL] Mask mismatch! Read: $READ_MASK, Expected: 7"
fi

echo "5. Testing invalid mask input '8' (should be rejected)..."
if echo 8 2>/dev/null | sudo tee "$SYSFS_FILE" >/dev/null 2>&1; then
    echo "   [FAIL] System accepted invalid mask '8'!"
    sudo rmmod "$MODULE"
    exit 1
else
    echo "   [OK] System successfully rejected invalid mask '8' (EINVAL)."
fi

echo "6. Letting it blink with mask '7' for 3 seconds..."
sleep 3

echo "7. Stopping blinking (writing '0')..."
echo 0 | sudo tee "$SYSFS_FILE" > /dev/null
echo "   [OK] Wrote '0' to stop."

echo "8. Checking kernel logs (dmesg) for blinking states..."
dmesg | tail -n 15 | grep -i "kbleds" || echo "   [INFO] No recent kbleds logs in dmesg."

echo "9. Removing module..."
sudo rmmod "$MODULE"

echo "10. Verifying sysfs file is removed..."
if [ ! -f "$SYSFS_FILE" ]; then
    echo "   [OK] $SYSFS_FILE successfully removed."
else
    echo "   [FAIL] $SYSFS_FILE still exists after rmmod!"
    exit 1
fi

echo "=== Task 3 Test Completed Successfully ==="