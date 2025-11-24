#!/bin/bash

# Benchmark script for 1.1
if [ $# -lt 2 ]; then
    echo "Usage: $0 <degree> <threads> [iterations] [output_file]"
    exit 1
fi

DEGREE=$1
THREADS=$2
ITERATIONS=$3
OUTPUT_FILE=${4:-"benchmark1_1_results.txt"} #if not provided, default to benchmark1_1_results.txt

make 1_1
if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

echo "=== Benchmark Results for Exercise 1.1 ===" > "$OUTPUT_FILE"
echo "Degree: $DEGREE" >> "$OUTPUT_FILE"
echo "Threads: $THREADS" >> "$OUTPUT_FILE"
echo "Iterations per test: $ITERATIONS" >> "$OUTPUT_FILE"
echo "========================================" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

echo "Running benchmark: n=$DEGREE, threads=$THREADS, iterations=$ITERATIONS"

serial_sum=0
parallel_sum=0
init_sum=0
for ((i=1; i<=$ITERATIONS; i++)); do
    echo -e "\n\n\n" >> "$OUTPUT_FILE"
    echo -n "Iteration $i/$ITERATIONS... ">> "$OUTPUT_FILE"
    ./1_1 $DEGREE $THREADS >> "$OUTPUT_FILE" 2>&1
done