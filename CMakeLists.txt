# Use of this file is subject to license terms as set forth in the LICENSE file found in the root directory of the project.

cmake_minimum_required(VERSION 2.6)
project(cnode)

if(USE_GCOV)
  set(CMAKE_BUILD_TYPE "Release")

  # Set global c and c++ flags
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")

  # Link flags used for creating executables
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgcov -fprofile-arcs")

  # Link flags used for creating shared libraries
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lgcov -profile-arcs")
endif()

# generic cmake configuration
include("cmake/cnode.cmake")



