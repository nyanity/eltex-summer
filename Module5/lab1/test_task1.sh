#!/bin/bash

MODULE="hello_module"
KO_FILE="${MODULE}.ko"

echo "=== Testing Task 1 (Hello World) ==="

if [ ! -f "$KO_FILE" ]; then
    echo "Error: $KO_FILE not found. Please run 'make' first."
    exit 1
fi

echo "1. Inserting module..."
sudo insmod "$KO_FILE"

echo "2. Checking if module is loaded..."
if lsmod | grep -q "$MODULE"; then
    echo "   [OK] Module is successfully loaded."
else
    echo "   [FAIL] Module is not loaded!"
    exit 1
fi

echo "3. Checking kernel logs (dmesg) for loading message..."
if dmesg | tail -n 20 | grep -q "Module loaded successfully"; then
    echo "   [OK] Found loading message in dmesg:"
    dmesg | tail -n 20 | grep "Module loaded successfully"
else
    echo "   [WARNING] Loading message not found in the last 20 lines of dmesg."
fi

echo "4. Removing module..."
sudo rmmod "$MODULE"

echo "5. Checking if module is unloaded..."
if ! lsmod | grep -q "$MODULE"; then
    echo "   [OK] Module is successfully unloaded."
else
    echo "   [FAIL] Module is still loaded!"
    exit 1
fi

echo "6. Checking kernel logs (dmesg) for unloading message..."
if dmesg | tail -n 20 | grep -q "Module unloaded successfully"; then
    echo "   [OK] Found unloading message in dmesg:"
    dmesg | tail -n 20 | grep "Module unloaded successfully"
else
    echo "   [WARNING] Unloading message not found in the last 20 lines of dmesg."
fi

echo "=== Task 1 Test Completed Successfully ==="