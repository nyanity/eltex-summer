#!/usr/bin/env bash
set -e

BUILD_BIN="./build/file_copy"

if [ ! -f "$BUILD_BIN" ]; then
    echo "Executable $BUILD_BIN not found! Build it first."
    exit 1
fi

echo "--- Test 1: Incorrect parameters (no files) ---"
if $BUILD_BIN > /dev/null 2>&1; then
    echo "FAILED: Program should have failed without arguments."
    exit 1
else
    echo "PASSED: Failed as expected."
fi

echo "--- Test 2: Non-existent file ---"
echo "Dummy" > valid_file.txt
$BUILD_BIN non_existent_file.txt valid_file.txt 2> err.log || true

if [ -f "valid_file.txt.copy" ]; then
    echo "PASSED: Copy created for valid file despite non-existent file."
else
    echo "FAILED: Copy for valid file was not created."
    rm -f valid_file.txt valid_file.txt.copy err.log
    exit 1
fi
rm -f valid_file.txt valid_file.txt.copy err.log

echo "--- Test 3: Multiple files with unnamed pipe ---"
echo "Content 1" > file1.txt
echo "Content 2" > file2.txt

$BUILD_BIN file1.txt file2.txt

if cmp -s file1.txt file1.txt.copy && cmp -s file2.txt file2.txt.copy; then
    echo "PASSED: Unnamed pipe copying works."
else
    echo "FAILED: Copies do not match originals."
    rm -f file1.txt file2.txt file1.txt.copy file2.txt.copy
    exit 1
fi
rm -f file1.txt file2.txt file1.txt.copy file2.txt.copy

echo "--- Test 4: Multiple files with named pipe ---"
echo "Content FIFO 1" > file1.txt
echo "Content FIFO 2" > file2.txt

$BUILD_BIN -p test_pipe_script file1.txt file2.txt

if cmp -s file1.txt file1.txt.copy && cmp -s file2.txt file2.txt.copy; then
    echo "PASSED: Named pipe copying works."
else
    echo "FAILED: Named pipe copies do not match originals."
    rm -f file1.txt file2.txt file1.txt.copy file2.txt.copy test_pipe_script
    exit 1
fi
rm -f file1.txt file2.txt file1.txt.copy file2.txt.copy test_pipe_script

echo "ALL TESTS PASSED!"