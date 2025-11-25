#!/bin/bash

# Benchmark script for 1.4
if [ $# -ne 6 ] && [ $# -ne 7 ]; then
    echo "Usage: $0 <number_of_accounts> <transactions_per_thread> <percentage_of_read_transactions> <type_of_sync> <number_of_threads>"
    echo  "Type_of_sync: mutex_coarse, mutex_fine, rwlock_coarse, rwlock_fine"
    exit 1
fi

N=$1
TRANSACTIONS_PER_THREAD=$2
PERCENTAGE_READ=$3
TYPE_OF_SYNC=$4
THREADS=$5
ITERATIONS=$6

OUTPUT_FILE=${7:-"benchmark1_4_results.txt"} #if not provided, default to benchmark1_4_results.txt

make 1_4
if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

echo "=== Benchmark Results for Exercise 1.4 ===" > "$OUTPUT_FILE"
echo "Number of accounts: $N" >> "$OUTPUT_FILE"
echo "Transactions per thread: $TRANSACTIONS_PER_THREAD" >> "$OUTPUT_FILE"
echo "Percentage of read transactions: $PERCENTAGE_READ" >> "$OUTPUT_FILE"
echo "Type of sync: $TYPE_OF_SYNC" >> "$OUTPUT_FILE"
echo "Threads: $THREADS" >> "$OUTPUT_FILE"
echo "Iterations per test: $ITERATIONS" >> "$OUTPUT_FILE"
echo "========================================" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"


for ((i=1; i<=$ITERATIONS; i++)); do
    echo -e "\n\n\n" >> "$OUTPUT_FILE"
    echo -n "Iteration $i/$ITERATIONS... ">> "$OUTPUT_FILE"
    ./1_4 $N $TRANSACTIONS_PER_THREAD $PERCENTAGE_READ $TYPE_OF_SYNC $THREADS >> "$OUTPUT_FILE" 2>&1
done