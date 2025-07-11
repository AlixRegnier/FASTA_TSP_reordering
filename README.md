# FASTA TSP reordering

This tool reorder a list of FASTA files and compute an order that is more compressible by solving a path TSP using the [Mash distance](https://mash.readthedocs.io/en/latest/distances.html).


The path TSP is estimated with the Nearest-Neighbor heuristic. It relies on a triangle matrix (still quadratic space complexity) and customized VPTree data structure for retrieving nearest neighbor that doesn't belong to current path.

For the very moment, parallelization is not implemented yet. It would drastically speed up VPTree initialization and Mash distance computations.

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