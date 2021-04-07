# Install script for directory: /lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libopenblas.so.0.2"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libopenblas.so.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libopenblas.so"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    endif()
  endforeach()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64" TYPE SHARED_LIBRARY FILES
    "/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/lib/libopenblas.so.0.2"
    "/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/lib/libopenblas.so.0"
    "/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/lib/libopenblas.so"
    )
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libopenblas.so.0.2"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libopenblas.so.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/libopenblas.so"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "${file}")
      endif()
    endif()
  endforeach()
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/openblas_config.h")
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/f77blas.h")
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/cblas.h")
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/lapack-netlib/LAPACKE/example/lapacke_example_aux.h"
    "/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/lapack-netlib/LAPACKE/include/lapacke.h"
    "/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/lapack-netlib/LAPACKE/include/lapacke_config.h"
    "/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/lapack-netlib/LAPACKE/include/lapacke_mangling.h"
    "/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/lapack-netlib/LAPACKE/include/lapacke_utils.h"
    )
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/lapacke_mangling.h")
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64" TYPE STATIC_LIBRARY FILES "/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/lib/libopenblas.a")
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64/pkgconfig" TYPE FILE FILES "/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/openblas.pc")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/interface/cmake_install.cmake")
  include("/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/driver/level2/cmake_install.cmake")
  include("/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/driver/level3/cmake_install.cmake")
  include("/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/driver/others/cmake_install.cmake")
  include("/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/kernel/cmake_install.cmake")
  include("/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/lapack/cmake_install.cmake")
  include("/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/utest/cmake_install.cmake")
  include("/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/test/cmake_install.cmake")
  include("/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/ctest/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/lustre/scratch3/turquoise/rvangara/RD100/distnnmfkcpp_Src/install_dependencies/xianyi-OpenBLAS-6d2da63/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
