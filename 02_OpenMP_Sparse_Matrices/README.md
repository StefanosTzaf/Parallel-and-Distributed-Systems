Running `make` compiles all programs and creates the executables in the `build` directory.

### **2.1 Polynomial Multiplication with OpenMP**
*Objective: Implement parallel polynomial multiplication utilizing OpenMP compiler directives to efficiently distribute the workload across shared-memory threads.*
* **To run the program:** `./build/2_1 <Degree of polynomial> <Number of threads>`
* **To run the script:** `./benchmark2_1.sh [degree of polynomial] [threads] [number of executions]`
  * The script defaults to 5 executions to calculate the average, so passing this argument is optional.
  * The results are saved in the `2_1_benchmark_results2.txt` file.

### **2.2 Sparse Matrix-Vector Multiplication (CSR) & Scheduling**
*Objective: Optimize memory and computation for sparse matrices using the Compressed Sparse Row (CSR) format. Explores the impact of different OpenMP runtime scheduling strategies (`static` vs. `guided`) under uniform and non-uniform workload distributions.*
* **To run the program:** `./build/2_2 <number of rows> <percentage of 0 elements> <iterations> <threads> [Y for non-uniform distribution]`
  * The last parameter is optional. If it is not provided, the distribution of non-zero elements is assumed to be uniform.
* **To run the script:**
  * The script is a Python program that generates files and CSV files containing the execution results inside the `bench_results` folder.
  * No parameters need to be passed to the script. It creates charts keeping certain parameters constant while varying the ones of interest.
  * **Note on Scheduling:** Because the scheduling is set to `runtime`, if you do not run the script (which automatically sets the `OMP_SCHEDULE` environment variable), you must set it manually before running the program. The experiments are conducted using `static` scheduling (and `guided` in the case of a non-uniform distribution).

### **2.3 Parallel Merge Sort (OpenMP Tasks)**
*Objective: Parallelize the top-down Merge Sort algorithm by leveraging OpenMP `task` directives to efficiently handle recursive parallel execution.*
* **To run the program:** `./build/2_3 <size of array> <serial or parallel> <number of threads>`
* **To run the script:** `./benchmark2_3.sh [size of array] [number of executions]`
  * The script defaults to an array size of 10^7 and 5 executions for calculating the average, so providing these arguments is optional. Furthermore, it runs the program sequentially, as well as in parallel for 1, 2, 4, and 8 threads, respectively.