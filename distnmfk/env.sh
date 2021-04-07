#!/bin/bash
export PATH="/usr/projects/w18_ntf_bda/ic_installs/openmpiV4.0.3/bin:$PATH"
export PATH="/usr/projects/w18_ntf_bda/ic_installs/pytorch_anaconda3/bin:$PATH"
export INCLUDE=$INCLUDE:/lustre/scratch4/turquoise/namita/rd100_dnnmfk_cpp-namita_edits/install_dependencies/xianyi-OpenBLAS-6d2da63/usr/local/include/:
export LIB=/lustre/scratch4/turquoise/namita/rd100_dnnmfk_cpp-namita_edits/install_dependencies/xianyi-OpenBLAS-6d2da63/usr/local/lib64/:
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/lustre/scratch4/turquoise/namita/rd100_dnnmfk_cpp-namita_edits/install_dependencies/xianyi-OpenBLAS-6d2da63/usr/local/lib64/:
export ARMADILLO_INCLUDE_DIR=/lustre/scratch4/turquoise/namita/rd100_dnnmfk_cpp-namita_edits/install_dependencies/armadillo-7.800.2/include/
export INCLUDE=$INCLUDE:$ARMADILLO_INCLUDE_DIR:
export NMFLIB_DIR=/lustre/scratch4/turquoise/namita/namitas_distnmfk/distnmfk_n/
export DISTNMF_SOURCE_DIR=/lustre/scratch4/turquoise/namita/namitas_distnmfk/planc-master/distnmf/
export INCLUDE=$INCLUDE:$ARMADILLO_INCLUDE_DIR:$NMFLIB_DIR
export CPATH=$CPATH:$INCLUDE
module load gcc openmpi
