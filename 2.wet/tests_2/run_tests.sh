#!/bin/bash

# Colors for better readability
GREEN="\033[0;32m"
RED="\033[0;31m"
NC="\033[0m" # No Color

echo "Running tests..."
echo

# Ensure necessary directories exist
mkdir -p YourOutputs
mkdir -p difflogs
echo "Created directories YourOutputs, difflogs"

# Gather and sort all tests/test*.command in version order, then iterate
printf "%s\n" tests/test*.command | sort -V | while read -r cmdpath; do
    # Extract the base test name (e.g., "test1" from "tests/test1.command")
    testname=$(basename "$cmdpath" .command)

    # Paths for our output, expected output, and diff log
    yours_out="YourOutputs/${testname}.YoursOut"
    expected="tests/${testname}.out"
    difflog="difflogs/${testname}.diff"

    echo -n "Running ${testname}... "

    # Run the .command file and capture its stdout into the subdirectory
    bash "$cmdpath" > "$yours_out"

    # Compare captured output with expected
    if diff "$yours_out" "$expected" > /dev/null; then
        # Test passed
        echo -e "${GREEN}PASSED${NC}"
        rm -f "$difflog"
    else
        # Test failed: read entire contents of expected and ours
        their_output=$(<"$expected")
        our_output=$(<"$yours_out")

        # Print FAILED in red
        echo -e "${RED}FAILED${NC} \nExpected: ${their_output} \nYourssss: ${our_output}"

        # Also save the diff into difflogs/
        diff "$yours_out" "$expected" > "$difflog"
    fi
done

echo
echo "All tests finished."
