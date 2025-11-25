#!/bin/bash

# Usage: ./benchmark1_2.sh [iterations] [runs]

ITERS=10000000 # 10^7 
RUNS=5  # default number of times to run each test
if [ $# -eq 2 ]; then
    ITERS=$1
    RUNS=$2
fi

# clear file for next run
truncate -s 0 1_2_benchmark_results.txt

# Redirect all output to file
exec >> 1_2_benchmark_results.txt 

echo "Compiling..."
make

echo ""
echo "Running with $ITERS iterations per thread (average of $RUNS runs)"
echo ""

# Function to run test multiple times and get average
run_test() {
    # arguments of ./1_2: threads, iterations, algorithm
    threads=$1
    iters=$2
    alg=$3
    
    sum=0
    for ((i=1; i<=$RUNS; i++)); do
        output=$(./build/1_2 $threads $iters $alg | grep "Time") # Extract line with time
        time=$(echo $output | cut -d' ' -f2) # Get only the time value
        sum=$(echo "$sum + $time" | bc) # calculate sum of times
    done
    
    avg=$(echo "scale=6; $sum / $RUNS" | bc) # division with 6 decimal digits
    echo "$threads threads: $avg seconds"
}

# Mutex with different thread counts
echo "=== MUTEX ==="
run_test 1 $ITERS 1
run_test 2 $ITERS 1
run_test 4 $ITERS 1
run_test 8 $ITERS 1

echo ""
echo "=== RWLOCK ==="
run_test 1 $ITERS 2
run_test 2 $ITERS 2
run_test 4 $ITERS 2
run_test 8 $ITERS 2

echo ""
echo "=== ATOMIC ==="
run_test 1 $ITERS 3
run_test 2 $ITERS 3
run_test 4 $ITERS 3
run_test 8 $ITERS 3
