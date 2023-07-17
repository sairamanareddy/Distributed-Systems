# Introduction and Problem Definition
- In the context of Distributed Computing, no process or node has complete knowledge of the global state.
- Therefore, determining if a distributed computation has terminated is a non-trivial problem.
- We define global termination as the scenario where every process/node has locally terminated and there is no message in transit between any processes/nodes.
- Termination detection is where a process or node must infer that the underlying computation has terminated.
- **In this particular project, we simulate a cell culture where cells may become infected as well as become immune to a virus. We have to determine if the culture has stabilized, i.e. all cells have become immune to the virus. The initial conditions are given such that this will eventually happen.**

# Algorithm
- There are many algorithms for detecting termination. However, in this project, we implement a spanning-tree based termination detection algorithm that is described in the [literature](https://www.cs.uic.edu/~ajayk/Chapter7.pdf). Please refer to the detailed [Problem Statement](https://github.com/sairamanareddy/Distributed-Systems/blob/master/Problem%20Statement.pdf) to learn more.

# Implementation
- We use MPI to simulate the whole system. MPI Processes will resemble cells in the culture
- Each process maintains its own state. (Namely, in this case, color of the cell and other variables required for the algorithm).
- Cell infections and immunizations are simulated by message passing between the processes
- Algorithm detects termination and measures statistics such as number of control messages used.

## To run the code
- Install OpenMPI and make sure you have the mpic++ and mpirun shell commands available.
```console
$ mpic++ <filename> (replace filename here)
$ mpirun -np <no. of processes> ./a.out (replace with no. of processes)
```

