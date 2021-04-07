#Copyright 2021 Gopinath Chennupati, Namita Kharat, Raviteja Vangara,  Erik Skau and Boian Alexandrov,
#Triad National Security, LLC. All rights reserved
#This program was produced under U.S. Government contract 89233218CNA000001 for Los Alamos National Laboratory (LANL), which is operated by Triad National #Security, LLC for the U.S. Department of Energy/National Nuclear Security Administration. All rights in the program are reserved by Triad National Security, #LLC, and the U.S. Department of Energy/National Nuclear Security Administration. The Government is granted for itself and others acting on its behalf a #nonexclusive, paid-up, irrevocable worldwide license in this material to reproduce, prepare derivative works, distribute copies to the public, perform publicly #and display publicly, and to permit others to do so.
# This CMakeLists.txt file is modified from Ramakrishna khannan's CMakeLists.txt file in Planc Library to build distnmfk with Planc directory given by DISTNMF_SOURCE_DIR 
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(ARMA_USE_WRAPPER true)

# the settings below will be automatically configured by the rest of this script

# set(ARMADILLO_LIB_DIR /home/okaya/armadillo-6.600.5/install/usr/lib)

project(DISTNMF CXX)
set(NMFLIB_COMMON ${DISTNMF_SOURCE_DIR}/../common)
set(NNLS_SRC ${DISTNMF_SOURCE_DIR}/../nnls/)
message(STATUS "DISTNMF_SOURCE_DIR =  ${DISTNMF_SOURCE_DIR}")
message(STATUS "NMFLIB_COMMON =  ${NMFLIB_COMMON}")

include(${NMFLIB_COMMON}/../../distnmfk/FindArmaNMFLib.cmake)

#BOOST package needed for activeset NNLS
#set(BOOST false)
#As advised by Wlliam Renaud note dated 4/22. There is an issue on Rhea
#in which the system boost is found before the version in modules.
#Ignore system boost and use module system boost
#set(Boost_NO_BOOST_CMAKE TRUE)
#find_package(Boost REQUIRED)

find_package(MPI REQUIRED)

message(STATUS " MPI_CXX_FOUND = ${MPI_CXX_FOUND}" )

if(MPI_CXX_FOUND)
  set(NMFLIB_LIBS ${NMFLIB_LIBS} ${MPI_CXX_LIBRARIES})
endif()

OPTION(CMAKE_BUILD_PACOSS "Build with PACOSS" OFF)
if(CMAKE_WITH_PACOSS)
  add_definitions(-DUSE_PACOSS=1)
  set(PACOSS_DIR ${DISTNMF_SOURCE_DIR}/../pacoss)
  set(PACOSS_INCLUDE_DIR ${DISTNMF_SOURCE_DIR}/../pacoss/src) 
  set(PACOSS_INCLUDE_DIR ${PACOSS_INCLUDE_DIR} ${DISTNMF_SOURCE_DIR}/../tmpi/src)
  set(PACOSS_LIB_DIR ${DISTNMF_SOURCE_DIR}/../pacoss/lib)
  link_directories(
    ${PACOSS_LIB_DIR}  
  )
  set(NMFLIB_LIBS "${NMFLIB_LIBS} pacoss")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -O0 -rdynamic")

message(STATUS "CMAKE_CXX_FLAGS           = ${CMAKE_CXX_FLAGS}"          )
message(STATUS "CMAKE_SHARED_LINKER_FLAGS = ${CMAKE_SHARED_LINKER_FLAGS}")
message(STATUS "CMAKE_REQUIRED_INCLUDES   = ${CMAKE_REQUIRED_INCLUDES}"  )

message(STATUS "")
message(STATUS "*** libraries:")
message(STATUS "*** NMFLIB_LIBS = ${NMFLIB_LIBS}")
message(STATUS "")

include_directories(
  ${PACOSS_INCLUDE_DIR}
  ${ARMADILLO_INCLUDE_DIR}
  ${ARMADILLO_INCLUDE_DIRS}
  ${DISTNMF_SOURCE_DIR}
  ${DISTNMF_SOURCE_DIR}/../
  ${NMFLIB_COMMON}
  ${NNLS_SRC}
  ${MPI_CXX_INCLUDE_PATH}
  ${MKL_INCLUDE_DIR}
  ${OPENBLAS_INCLUDE_DIR}
)

add_executable(distnmfk
  distnmfk.cpp
)

add_definitions(-std=c++11 -w)

if(CMAKE_WITH_PACOSS)
  target_link_libraries(distnmfk ${NMFLIB_LIBS} pacoss)
else()
  target_link_libraries(distnmfk ${NMFLIB_LIBS})
endif()
