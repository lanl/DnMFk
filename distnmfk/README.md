Distributed NMFk
=============================

Install Instructions
--------------------

The necessary dependecies are included in this directory.

Install dnnmfk by using the script _installDistnmfk.sh_

````
bash installDistnmfk.sh
````

The target executable can be found in the distnmfk directory


General Install Instructions
--------------------
This program depends on:

- Download Armadillo library which can be found at https://arma.sourceforge.net
- Download and build OpenBLAS https://github.com/xianyi/OpenBLAS

Once the above steps are completed, set the following environment variables.

````
export ARMADILLO_INCLUDE_DIR=/home/rnu/libraries/armadillo-6.600.5/include/
export LIB=$LIB:/home/rnu/libraries/openblas/lib:
export INCLUDE=$INCLUDE:/home/rnu/libraries/openblas/include:$ARMADILLO_INCLUDE_DIR:
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/rnu/libraries/openblas/lib/:
export NMFLIB_DIR=/ccs/home/ramki/rhea/research/nmflib/
export INCLUDE=$INCLUDE:$ARMADILLO_INCLUDE_DIR:$NMFLIB_DIR
export CPATH=$CPATH:$INCLUDE:
export MKLROOT=/ccs/compilers/intel/rh6-x86_64/16.0.0/mkl/
````

If you have got MKL, please source MKLVARS.sh before running make/cmake

For Tensor networks Group on Grizzly
------------------------------------
````
export PATH="/usr/projects/w18_ntf_bda/ic_installs/openmpiV4.0.3/bin:$PATH"
export PATH="/usr/projects/w18_ntf_bda/ic_installs/pytorch_anaconda3/bin:$PATH"
export ARMADILLO_INCLUDE_DIR=/usr/projects/w18_ntf_bda/ic_installs/armadillo7.8/include/
export LIB=$LIB:/usr/projects/w18_ntf_bda/ic_installs/openblas/lib:
export INCLUDE=$INCLUDE:/usr/projects/w18_ntf_bda/ic_installs/openblas/include:$ARMADILLO_INCLUDE_DIR:
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/projects/w18_ntf_bda/ic_installs/openblas/lib:
export NMFLIB_DIR=/usr/projects/w18_ntf_bda/namsk/cgnath/cp_decompose/conv_planc/distnmfk/
export INCLUDE=$INCLUDE:$ARMADILLO_INCLUDE_DIR:$NMFLIB_DIR
export CPATH=$CPATH:$INCLUDE
export MKLROOT=/usr/projects/hpcsoft/toss3/common/intel-clusterstudio/2017.1.024/compilers_and_libraries_2017.1.132/linux/mkl/

````



Sparse NMF
---------
Run cmake with -DCMAKE_BUILD_SPARSE

Sparse Debug build
------------------
Run cmake with -DCMAKE_BUILD_SPARSE -DCMAKE_BUILD_TYPE=Debug

Building on Cray-EOS/Titan
-----------------------
CC=CC CXX=CC cmake ~/nmflibrary/distnmf/ -DCMAKE_IGNORE_MKL=1

Building on Titan with NVBLAS
-----------------------------
We are using NVBLAS to offload computations to GPU.
By default we enable building with cuda in Titan.
The sample configurations files for nvblas can be found at conf/nvblas_cuda75.conf
and conf/nvblas_cuda91.conf for CUDA Toolkit 7.5 and 9.1 respectively.

CC=CC CXX=CC cmake ~/nmflibrary/distnmf/ -DCMAKE_IGNORE_MKL=1 -DCMAKE_BUILD_CUDA=1


Other Macros
-------------

* CMAKE macros

  For sparse NMF - cmake -DBUILD_SPARSE=1 - Default dense build
  For timing with barrier after mpi calls - cmake -DCMAKE_WITH_BARRIER_TIMING - Default with barrier timing
  For performance, disable the WITH__BARRIER__TIMING. Run as "cmake -DCMAKE_WITH_BARRIER_TIMING:BOOL=OFF"
  For building cuda - -DCMAKE_BUILD_CUDA=1 - Default is off.

* Code level macros - Defined in distutils.h

  MPI_VERBOSE - Be doubly sure about what you do. It prints all intermediary matrices.
			   Try this only for very very small matrix that is of size less than 10.
  WRITE_RAND_INPUT - for dumping the generated random matrix

Output interpretation
======================
For W matrix row major ordering. That is., W_0, W_1, .., W_p
For H matrix column major ordering. That is., for 6 processes
with pr=3, pc=2, interpret as H_0, H_2, H_4, H_1, H_3, H_5

Running
=======
````
mpirun -np p "distnmfk" -a $i -k $k -i "random" -t $j -d "240 180" -p "p 1" -o "nmfout_${out}" -r "0.001 0 0.001 0" -e 1 
````
````
p : Number of Processors/cores
a : algorithm
k : For k, rank of NMF
i : input type or data location
t : Number of iterations
d : Dimensions of the matrix to be decomposed
o : directory to produce output files
e : Flag to print relative error 
````
Citation:
=========

If you are using this implementation, kindly cite.

<div class="csl-entry">Chennupati, G., Vangara, R., Skau, E., Djidjev, H., &amp; Alexandrov, B. (2020). Distributed non-negative matrix factorization with determination of the number of latent features. <i>The Journal of Supercomputing</i>, <i>76</i>(9), 7458–7488. <a href="https://doi.org/10.1007/s11227-020-03181-6">https://doi.org/10.1007/s11227-020-03181-6</a></div>
  <span class="Z3988" title="url_ver=Z39.88-2004&amp;ctx_ver=Z39.88-2004&amp;rfr_id=info%3Asid%2Fzotero.org%3A2&amp;rft_id=info%3Adoi%2F10.1007%2Fs11227-020-03181-6&amp;rft_val_fmt=info%3Aofi%2Ffmt%3Akev%3Amtx%3Ajournal&amp;rft.genre=article&amp;rft.atitle=Distributed%20non-negative%20matrix%20factorization%20with%20determination%20of%20the%20number%20of%20latent%20features&amp;rft.jtitle=The%20Journal%20of%20Supercomputing&amp;rft.stitle=J%20Supercomput&amp;rft.volume=76&amp;rft.issue=9&amp;rft.aufirst=Gopinath&amp;rft.aulast=Chennupati&amp;rft.au=Gopinath%20Chennupati&amp;rft.au=Raviteja%20Vangara&amp;rft.au=Erik%20Skau&amp;rft.au=Hristo%20Djidjev&amp;rft.au=Boian%20Alexandrov&amp;rft.date=2020-09&amp;rft.pages=7458-7488&amp;rft.spage=7458&amp;rft.epage=7488&amp;rft.issn=0920-8542%2C%201573-0484&amp;rft.language=en"></span>
