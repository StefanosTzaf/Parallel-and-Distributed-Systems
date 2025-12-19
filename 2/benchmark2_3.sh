#!/bin/bash

# Usage: ./benchmark2_3.sh <array size> <number of executions>

SIZE=1000000  # Default number of array size 
RUNS=5  # Default number of runs per test
ALG="parallel"
if [ $# -eq 2 ]; then
    SIZE=$1
    RUNS=$2
fi

# clear file for next run
truncate -s 0 2_3_benchmark_results.txt

# Redirect all output to file
exec >> 2_3_benchmark_results.txt 

echo "Compiling..."
gcc -g -Wall -fopenmp -o 2_3 2_3.c

echo ""
echo "Running with $SIZE array size(average of $RUNS runs)"
echo ""

# Function to run test multiple times and get average
run_test_1() {
    # arguments of ./2_3: <array size> <algorithm> <threads>
    threads=$3
    alg=$2
    size=$1
    
    sum=0
    for ((i=1; i<=$RUNS; i++)); do
        output=$(./2_3 $size $alg $threads | grep "Time") # Extract line with time
        time=$(echo $output | cut -d' ' -f2) # Get only the time value (2nd field)
        sum=$(echo "$sum + $time" | bc) # calculate sum of times
    done
    
    avg=$(echo "scale=6; $sum / $RUNS" | bc) # division with 6 decimal digits

    if [ "$alg" == "parallel" ]; then
        echo "$threads threads: $avg seconds"
    else
        echo "Serial execution: $avg seconds"
    fi

    
}

if [ "$ALG" == "parallel" ]; then

    run_test_1 $SIZE $ALG 1
    run_test_1 $SIZE $ALG 2
    run_test_1 $SIZE $ALG 4
    run_test_1 $SIZE $ALG 8

fi

ALG="serial"
run_test_1 $SIZE $ALG 1




