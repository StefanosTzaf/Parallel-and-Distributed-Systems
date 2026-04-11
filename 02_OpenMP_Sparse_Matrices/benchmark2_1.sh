#!/bin/bash

# Usage: ./benchmark2_1.sh [degree] [threads] [runs]

# Benchmark script for 2.1
if [ $# -lt 2 ]; then
    echo "Usage: $0 <degree> <threads> [number of executions]"
    exit 1
fi

# clear file for next run
truncate -s 0 2_1_benchmark_results2.txt

# Redirect all output to file
exec >> 2_1_benchmark_results2.txt 

DEGREE=$1
THREADS=$2
RUNS=${3:-5} # default to 5 runs if not provided

make 

if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

echo "=== Benchmark Results for Exercise 2.1 ===" 
echo "Degree: $DEGREE" 
echo "Threads: $THREADS" 
echo "Number of executions per test: $RUNS" 
echo "========================================" 
echo "" 

echo "Running benchmark: n=$DEGREE, threads=$THREADS, runs=$RUNS"

serial_sum=0
parallel_sum=0
init_sum=0
speedup_sum=0
efficiency_sum=0
for ((i=1; i<=$RUNS; i++)); do

    echo "execution $i/$RUNS..."
   
    output=$(./build/2_1 $DEGREE $THREADS)

    initOutput=$(echo "$output" | grep "Initialization time") # Extract line with time
    serialOutput=$(echo "$output" | grep "Serial algorithm time")
    parallelOutput=$(echo "$output" | grep "Parallel algorithm time") 
    speedupOutput=$(echo "$output" | grep "Speedup") 
    efficiencyOutput=$(echo "$output" | grep "Efficiency")
    
    initTime=$(echo $initOutput | cut -d' ' -f3) # Get only the time value
    serialTime=$(echo $serialOutput | cut -d' ' -f4) 
    parallelTime=$(echo $parallelOutput | cut -d' ' -f4) 
    speedup=$(echo $speedupOutput | cut -d' ' -f2)
    efficiency=$(echo $efficiencyOutput | cut -d' ' -f2)

    init_sum=$(echo "$init_sum + $initTime" | bc) # calculate sum of times
    serial_sum=$(echo "$serial_sum + $serialTime" | bc) 
    parallel_sum=$(echo "$parallel_sum + $parallelTime" | bc)
    speedup_sum=$(echo "$speedup_sum + $speedup" | bc) 
    efficiency_sum=$(echo "$efficiency_sum + $efficiency" | bc) 

    correct_results=$(echo "$output" | grep "The results are CORRECT!")
    if [ -z "$correct_results" ]; then
        echo "Warning: Results are NOT CORRECT in execution $i!"
    else
        echo "Results are CORRECT in execution $i."
    fi

done

avg_init=$(echo "scale=6; $init_sum / $RUNS" | bc) # division with 6 decimal digits
avg_serial=$(echo "scale=6; $serial_sum / $RUNS" | bc) 
avg_parallel=$(echo "scale=6; $parallel_sum / $RUNS" | bc) 
avg_speedup=$(echo "scale=6; $speedup_sum / $RUNS" | bc) 
avg_efficiency=$(echo "scale=6; $efficiency_sum / $RUNS" | bc)

echo ""
echo "Average Initialization time: $avg_init s"
echo "Average Serial algorithm time: $avg_serial s"
echo "Average Parallel algorithm time: $avg_parallel s"
echo "Average Speedup: $avg_speedup x"
echo "Average Efficiency: $avg_efficiency %"
