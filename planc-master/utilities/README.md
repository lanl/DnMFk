In this directory, I provide the sample small utilities to build the real world datasets.

1. 1D Split of Dense Data - Have a space separated matrix and its transpose in the same directory. Run 
````splitfiles.sh filename filenamet #cores```` 
The above command will generate appropriate directories and dump the 1D distribution of the matrix.
2. 2D Split of Dense data - Have a space separated matrix alone. You DON'T need the matrix transpose.
Run ````splitfiles.sh filename filenamet #cores````. 
The above command will prepare the data that can be given as input to the nmf program.
3. 1D or 2D split of sparse file - The input file must be in coordinate format with zero indexing.
That is the index starts with zero. To run this command, first your have to build SplitFiles.cpp.
A sample build file is provided as buildsplitfiles.sh. 

````
Usage 1 : SplitFiles inputmtxfile outputdirectory numsplits [pr=1] [pc=1]
Usage 2 : SplitFiles m n density seed numsplits outputdirectory
````

In the case of Usage 1, if you don't specify pr, pc it will generate a 1D split and 2D otherwise.
The usage 2, generates splits for 1D and 2D random sparse matrix. The usage 2 may not be needed
for this library as it can generate synthetic matrix on the fly in memory. 

4. The file processstackexchange.txt, gives the shell command listing for converting
the .xml file into a .txt file that contains one question every line. This can then be
consumed into mangodb or use some python scripting tool to build the sparse bag of words 
matrix. The sparse bag of words matrix can be uniformly split using the splitfiles utility
explained above. 

5. shufflesparsemm.py shuffles the sparse matrix in coordinate format for load balancing.
It permutes both the rows and the columns of the matrix. It can be run as 

````
python shufflesparsemm.py -i inputfile -o outputfile
````

Once completed running it generates three files. Shuffled matrix file and the
outputfile_rowperm as the row permutation indexes and the outputfile_colperm as
col permutation indexes