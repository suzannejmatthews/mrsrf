## MrsRF
Welcome to the new homepage of MrsRF. The previous homepage was [mrsrf.googlecode.com](https://code.google.com/archive/p/mrsrf/). 
MrsRF stands for (M)ap (R)educe (S)peeds up (R)obinson (F)oulds. It is pronounced "Missus Are-Eff". MrsRF is a scalable, 
efficient multi-core algorithm that uses MapReduce to quickly calculate the all-to-all Robinson Foulds (RF) distance between 
large numbers of trees. For t trees, this is outputted as a t x t matrix.

MrsRF can run on multiple nodes and multiple cores. It can even be executed sequentially. MrsRF currently works on linux 
distributions. We have tested MrsRF on the CentOS and Ubuntu platforms. MrsRF is built on top of the Phoenix MapReduce framework.

## Compiling Code
To create executables, type `make` in the `src/` directory, and the `hashtable/` directory. 
All MrsRF executables are stored in the `src/` directory. hashbase's executable is located in `hashbase/`.

### Executable descriptions:  

`mrsrf-pq`: 
  - Used in conjunction with mrsrf.
  - Creates a matrix of size pxq.

`mrsrf`:
  - Works on k nodes to create a t x t matrix.
  - Calls mrsrf-pq to create RF submatrices.


## Usage
To run MrsRF on `N` nodes and `c` cores, run the following command in the main directory:

```bash
mpirun -np <N> src/mrsrf <cores> <input file> <number of taxa> <number of trees> <output> <rounds>
```

Where:
<N> represents the number of nodes
<cores> represents the number of cores
<input file> is the input file to the program
<number of taxa> number of taxa represented by the input file
<number of trees> represents the number of the trees in the input file
<output> either 0 or 1. Use "0" if you do not want the matrix to be 
printed out. Use "1" to print out the matrix.
<rounds> Specifies the number of iterations. For all practical purposes, 
please use 1 as the value.

For example, to run mrsrf on 2 nodes and 4 cores, using an input file 
specified by "test.tre" which contains 12 trees with 10 taxa each, run 
the following:

```bash
mpirun -np 2 src/mrsrf 4 test.tre 10 12 1 1 
```

### Other notes:
If the code does not compile out of box, it may be necessary to reset the CPU_SET definitions. 
Please comment out the code associated with "new definitions" and uncomment the code section associated with "old definitions". This 
should be located near the top of `src/MapReduceScheduler.c`.

## To Cite:
Suzanne J Matthews and Tiffani L Williams. "MrsRF: An efficient MapReduce algorithm for analyzing large collections of 
evolutionary trees". BMC Bioinformatics 2010, 11(Suppl 1):S15. [Read the paper](http://bmcbioinformatics.biomedcentral.com/articles/10.1186/1471-2105-11-S1-S15).
