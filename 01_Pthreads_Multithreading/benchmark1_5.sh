#!/bin/bash

# Usage: ./benchmark1_5.sh [iterations] [runs]

ITERS=1000000  # Default number of iterations per thread
RUNS=5  # Default number of runs per test
if [ $# -eq 2 ]; then
    ITERS=$1
    RUNS=$2
fi

# clear file for next run
truncate -s 0 1_5_benchmark_results.txt

# Redirect all output to file
exec >> 1_5_benchmark_results.txt 

echo "Compiling..."
make

echo ""
echo "Running with $ITERS iterations per thread (average of $RUNS runs)"
echo ""

# Function to run test multiple times and get average
run_test_1() {
    # arguments of ./1_5: threads, iterations
    threads=$1
    iters=$2
    
    sum=0
    for ((i=1; i<=$RUNS; i++)); do
        output=$(./build/1_5 $threads $iters | grep "Elapsed time") # Extract line with time
        time=$(echo $output | cut -d' ' -f3) # Get only the time value (3rd field)
        sum=$(echo "$sum + $time" | bc) # calculate sum of times
    done
    
    avg=$(echo "scale=6; $sum / $RUNS" | bc) # division with 6 decimal digits
    echo "$threads threads: $avg seconds"
}

run_test_2() {
    # arguments of ./1_5_mutex_condvar: threads, iterations
    threads=$1
    iters=$2
    
    sum=0
    for ((i=1; i<=$RUNS; i++)); do
        output=$(./build/1_5_mutex_condvar $threads $iters | grep "Elapsed time") # Extract line with time
        time=$(echo $output | cut -d' ' -f3) # Get only the time value (3rd field)
        sum=$(echo "$sum + $time" | bc) # calculate sum of times
    done
    
    avg=$(echo "scale=6; $sum / $RUNS" | bc) # division with 6 decimal digits
    echo "$threads threads: $avg seconds"
}

run_test_3() {
    # arguments of ./1_5_busy_waiting: threads, iterations
    threads=$1
    iters=$2
    
    sum=0
    for ((i=1; i<=$RUNS; i++)); do
        output=$(./build/1_5_busy_waiting $threads $iters | grep "Elapsed time") # Extract line with time
        time=$(echo $output | cut -d' ' -f3) # Get only the time value (3rd field)
        sum=$(echo "$sum + $time" | bc) # calculate sum of times
    done
    
    avg=$(echo "scale=6; $sum / $RUNS" | bc) # division with 6 decimal digits
    echo "$threads threads: $avg seconds"
}

# default barrier with different thread counts
echo "=== PTHREAD BARRIER ===" 
run_test_1 1 $ITERS
run_test_1 2 $ITERS
run_test_1 4 $ITERS


# mutex and condition variable barrier with different thread counts
echo ""
echo "=== MUTEX & CONDVAR BARRIER ==="
run_test_2 1 $ITERS
run_test_2 2 $ITERS
run_test_2 4 $ITERS


# busy waiting barrier with different thread counts
echo ""
echo "=== BUSY WAITING BARRIER ==="
run_test_3 1 $ITERS
run_test_3 2 $ITERS
run_test_3 4 $ITERS





