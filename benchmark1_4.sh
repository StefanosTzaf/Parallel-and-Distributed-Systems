#!/bin/bash

# Benchmark script for 1.4 - no type of sync sync is given we will run it for all 4 types
if [ $# -ne 5 ] && [ $# -ne 4 ]; then
    echo "Usage: $0 <number_of_accounts> <transactions_per_thread> <percentage_of_read_transactions> <number_of_threads> [iterations]"
    echo "Do not provide type of sync, the script will run all 4 types."
    exit 1
fi

if  [[ "$1" =~ ^[a-zA-Z_]+$ ]]||  [[ "$2" =~ ^[a-zA-Z_]+$ ]] ||  [[ "$3" =~ ^[a-zA-Z_]+$ ]] ||  [[ "$4" =~ ^[a-zA-Z_]+$ ]] ||  [[ "$5" =~ ^[a-zA-Z_]+$ ]]; then
    echo "Usage: $0 <number_of_accounts> <transactions_per_thread> <percentage_of_read_transactions> <number_of_threads> [iterations]"
    echo "Do not provide type of sync, the script will run all 4 types."
    exit 1
fi



N=$1
TRANSACTIONS_PER_THREAD=$2
PERCENTAGE_READ=$3
THREADS=$4
ITERATIONS=${5:-5} # default to 5 iterations if not provided

# Redirect all output to file
truncate -s 0 1_4_benchmark_results.txt
exec >> 1_4_benchmark_results.txt

make 1_4
if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

echo "=== Benchmark Results for Exercise 1.4 ==="
echo "Number of accounts: $N"
echo "Transactions per thread: $TRANSACTIONS_PER_THREAD"
echo "Percentage of read transactions: $PERCENTAGE_READ"
echo "Threads: $THREADS"
echo "Iterations per test: $ITERATIONS"
echo "========================================"
echo ""

sync_types=("mutex_coarse" "mutex_fine" "rwlock_coarse" "rwlock_fine")

# Run benchmark for each sync type
for sync_type in "${sync_types[@]}"; do
    echo "=== Testing with $sync_type ==="
    echo ""
    
    time_sum=0
    
    for ((i=1; i<=$ITERATIONS; i++)); do
        echo "Execution $i/$ITERATIONS for $sync_type..."
        
        output=$(./build/1_4 $N $TRANSACTIONS_PER_THREAD $PERCENTAGE_READ $sync_type $THREADS 2>&1)
        
        time_string=$(echo "$output" | grep "Total time for transactions with")
        time_value=$(echo "$time_string" | cut -d' ' -f7)
        
        time_sum=$(echo "$time_sum + $time_value" | bc)
        
        
        correct_results=$(echo "$output" | grep "Error: Sum of balances changed!")
        if [ -n "$correct_results" ]; then
            echo "Error detected in execution $i for $sync_type!"
        else
            echo "Execution $i for $sync_type completed successfully."
        fi
    done
    
    avg_time=$(echo "scale=6; $time_sum / $ITERATIONS" | bc)
    
    echo ""
    echo "--- Average Results for $sync_type ---"
    echo "Average Time: $avg_time seconds over $ITERATIONS runs"
    echo ""
    echo "========================================================="
    echo ""
done

echo "=== Benchmark Complete ==="