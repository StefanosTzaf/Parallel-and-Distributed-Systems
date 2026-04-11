Running `make` compiles all programs using the `mpicc` wrapper and creates the executables in the `build` directory.

### **3.1 Distributed Polynomial Multiplication**
*Objective: Distribute the computational workload of polynomial multiplication across multiple MPI processes in a distributed memory environment. Process 0 handles data initialization, workload distribution, and gathers the final results.*
* **To run the program:** `mpiexec -n <processes> ./build/3_1 <Degree of polynomial>`
* **To run the script:** `python3 benchmark3_1.py`
  * The script defaults to 4 executions to calculate the average.
  * The results are saved in the bench_results folder.

### **3.2 Distributed Sparse Matrix-Vector Multiplication (CSR)**
*Objective: Perform parallel multiplication of a sparse matrix with a vector using the Compressed Sparse Row (CSR) format across distributed nodes. Evaluates the scaling and communication overhead (data scattering/gathering) of MPI.*
* **To run the program:** `mpiexec -n <processes> ./build/3_2 <number of rows> <percentage of 0 elements> <iterations>`
* **To run the script:** `python3 benchmark3_2.py`
  * The script defaults to 4 executions for calculating the average. 
  * Results and execution times are logged in the bench_results folder.

### General Notes:
MPI_instructions folder is about how to set up and run MPI programs on a cluster (specifically on my university's labs).
So you can just ignore it. Work.sh is a script that checks how much loaded the cluster is and it works against the university's cluster. You can also ignore it.
