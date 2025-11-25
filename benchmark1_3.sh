#!/bin/bash

#!/bin/bash

# Usage: ./benchmark1_3.sh [size of array] [runs]

SIZE=100000 # default size of array: 10^5
RUNS=10  # default number of times to run each test
if [ $# -eq 2 ]; then
    SIZE=$1
    RUNS=$2
fi


echo "Compiling..."
make

echo ""
echo "Running with array size $SIZE (average of $RUNS runs)"
echo ""

# Function to run test multiple times and get average
run_test() {
    # arguments of ./1_3: size of array
    size=$1
    
    sumParal=0
    sumSerial=0
    for ((i=1; i<=$RUNS; i++)); do
       
        outputSerial=$(./1_3 $size | grep "Serial Execution Time") # Extract line with serial time
        outputParal=$(./1_3 $size | grep "Parallel Execution Time") # Extract line with parallel time

        timeSerial=$(echo $outputSerial | cut -d' ' -f4) # Get only the time value
        timeParal=$(echo $outputParal | cut -d' ' -f4) # Get only the time value

        sumSerial=$(echo "$sumSerial + $timeSerial" | bc) # calculate sum of serial times
        sumParal=$(echo "$sumParal + $timeParal" | bc) # calculate sum of parallel times
    done
    
    avgSerial=$(echo "scale=6; $sumSerial / $RUNS" | bc) # division with 6 decimal digits
    avgParal=$(echo "scale=6; $sumParal / $RUNS" | bc) # division with 6 decimal digits
  
    echo "for array size $size:"
    echo "Average Serial Execution Time: $avgSerial seconds"
    echo "Average Parallel Execution Time: $avgParal seconds"
    echo ""
}

# Run the test
run_test $SIZE
