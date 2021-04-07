# - Config file for the Armadillo package
# It defines the following variables
#  ARMADILLO_INCLUDE_DIRS - include directories for Armadillo
#  ARMADILLO_LIBRARY_DIRS - library directories for Armadillo (normally not used!)
#  ARMADILLO_LIBRARIES    - libraries to link against

# Tell the user project where to find our headers and libraries
set(ARMADILLO_INCLUDE_DIRS "/lustre/scratch4/turquoise/rvangara/RD100exps/distnnmfkcpp_Src/install_dependencies/armadillo-7.800.2;/lustre/scratch4/turquoise/rvangara/RD100exps/distnnmfkcpp_Src/install_dependencies/armadillo-7.800.2")
set(ARMADILLO_LIBRARY_DIRS "/lustre/scratch4/turquoise/rvangara/RD100exps/distnnmfkcpp_Src/install_dependencies/armadillo-7.800.2")

# Our library dependencies (contains definitions for IMPORTED targets)
include("/lustre/scratch4/turquoise/rvangara/RD100exps/distnnmfkcpp_Src/install_dependencies/armadillo-7.800.2/ArmadilloLibraryDepends.cmake")

# These are IMPORTED targets created by ArmadilloLibraryDepends.cmake
set(ARMADILLO_LIBRARIES armadillo)

