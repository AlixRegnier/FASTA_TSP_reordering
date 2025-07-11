# FASTA TSP reordering

This tool reorder a list of FASTA files and compute an order that is more compressible by solving a path TSP using the [Mash distance](https://mash.readthedocs.io/en/latest/distances.html).


The path TSP is estimated with the Nearest-Neighbor heuristic. It relies on a triangle matrix (still quadratic space complexity) and customized VPTree data structure for retrieving nearest neighbor that doesn't belong to current path.

For the very moment, **parallelization is not implemented yet**. It would drastically speed up VPTree initialization and Mash distance computations.

Mash program used: https://github.com/marbl/Mash

## Installation

### Container 
```bash
git clone https://github.com/AlixRegnier/FASTA_TSP_reordering.git
cd FASTA_TSP_reordering

#Build container
apptainer build container.sif recipe.def

#Compile
apptainer exec container.sif make
```

### Manual

If you have ``g++`` and ``mash`` in your ``$PATH``, you can get rid of the container.

```bash
git clone https://github.com/AlixRegnier/FASTA_TSP_reordering.git
cd FASTA_TSP_reordering

#Compile
make

#Use program
./reorder ...
```

## Usage

When executing the program you need to have ``mash`` in your ``$PATH``, if you are using the program through the container, you will be able to use it directly since ``mash`` is installed in the container. This needed because the program will call mash through a child process.

### Memory 
**Make sure you have enough memory (RAM)**, required memory is almost upper bounded by the number of FASTA files:  

Here is an estimation of memory needed according to the groupsize, if you use ``groupsize = 0``, use the total number of FASTA files.

groupsize|Memory
--:|--:
100 | 50 kiB
1,000 | 5 MiB
10,000 | 500 MiB
100,000 | 50 GiB
1,000,000 | 5 TiB

**Estimation formula:**
$$\text{memory} \approx (4n^2 + 2n)\,\,\text{bytes}$$

Current implementation use a triangle matrix for storing distances. Each distance takes 8 bytes (double type).

### Time complexity

* Best-case time complexity: $\mathcal{O}(n.\text{log}_2(n))$
* Average-case time complexity: totally depends on the data.. further investigations are needed
* Worst-case time complexity: $\mathcal{O}(n^2)$

If all DNA sequences are all completely unrelated, the worst-case will happen because VPTree won't be able to efficiently determine where to search the nearest-neighbor and will need to compute each distances.

If you have only one group or several groups, each containing similar elements, VPTree should fall in best-case scenerio.

### Arguments

Program will check if both FOF have same size; make sure that FASTA file order corresponds to sketches order. In fact, you can also put same FOF for ``fof_fa`` and ``fof_msh`` but this will be slower because each distance computation will sketch both given FASTA files over and over.

```
Usage: reorder <fof_fa> <fof_msh> <groupsize> <output>

fof_fa		File of files containing paths to FASTA files
fof_msh		File of files containing paths to sketches
groupsize	Batch FASTA files into groups of size <groupsize> (last group may be smaller), put 0 if you want to create only one group containing all FASTA files
output		Output file of files, a permutation of <fof_fa> according to path TSP
```

### Container
If using the container, make sure to mount the directory where is the binary of the program and data directory. Otherwise, you can copy the binary to your working directory.

* No shell
```bash
apptainer exec --bind /path/to/binary/dir,/path/to/data /path/to/container/image /path/to/binary/dir/reorder fof.txt fof_msh.txt 0 output.txt
```

* Interactive shell
```bash
apptainer shell --bind /path/to/binary/dir,/path/to/data /path/to/container/image
/path/to/binary/dir/reorder fof.txt fof_msh.txt 0 output.txt
```

## References

* VPTree  

    > Yianilos, Peter N.. “Data structures and algorithms for nearest neighbor search in general metric spaces.” ACM-SIAM Symposium on Discrete Algorithms (1993).

* Mash
    > Ondov, Brian D. et al. “Mash: fast genome and metagenome distance estimation using MinHash.” Genome Biology 17 (2015): n. pag.