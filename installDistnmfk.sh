#unzip cp_decompose.zip
#tar -xvf armadillo-7.800.2.tar.xz
#tar -xvf OpenBLAS\ 0.2.20\ version.tar.gz
module load gcc openmpi cmake/3.9.0
echo "Building OpenBlas"
cd install_dependencies/xianyi-OpenBLAS-6d2da63/
echo "Now in Directory : ${PWD}"
cmake .
echo "Installing OpenBlas"
make -j32
make PREFIX=$PWD/ DESTDIR=$PWD/ install -j32
echo "Linking installed OpenBlas directories ..."
export INCLUDE=$INCLUDE:$PWD/usr/local/include/:
export LIB=$PWD/usr/local/lib64/:
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/usr/local/lib64/:
echo "Building and Installing Armadillo"
cd ../armadillo-7.800.2/
cmake .
make -j32
make install DESTDIR=$PWD/
echo "Build and install distributed nmfk"
export ARMADILLO_INCLUDE_DIR=$PWD/include/
export INCLUDE=$INCLUDE:$ARMADILLO_INCLUDE_DIR:
cd ../../distnmfk/
export NMFLIB_DIR=$PWD/
export INCLUDE=$INCLUDE:$ARMADILLO_INCLUDE_DIR:$NMFLIB_DIR
export CPATH=$CPATH:$INCLUDE
module load gcc openmpi
rm -r CMakeFiles/
rm CMakeCache.txt
rm cmake_install.cmake
rm Makefile
pathtodistnmf="$PWD/../planc-master/distnmf"
cmake $PWD/ -DCMAKE_IGNORE_MKL=1 -DCMAKE_BUILD_CUDA=0 -DDISTNMF_SOURCE_DIR=$pathtodistnmf
make -f CMakeFiles/distnmfk.dir/build.make CMakeFiles/distnmfk.dir/build
echo "distnmfk: Check if distnmfk executable is in ${PWD}"
