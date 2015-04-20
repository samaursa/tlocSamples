#------------------------------------------------------------------------------
# This file is included AFTER CMake adds the executable/library
# Do NOT remove the following variables. Modify the variables to suit your 
# project.

# Do NOT remove the following variables. Modify the variables to suit your project
set(SOLUTION_SOURCE_FILES
  main.cpp
  )

# Do not include individual assets here. Only add paths
set(SOLUTION_ASSETS_PATH
  ../../assets
  )

# Dependent project is compiled after dependency
set(SOLUTION_PROJECT_DEPENDENCIES
  )

# Libraries that the executable needs to link against
set(SOLUTION_EXECUTABLE_LINK_LIBRARIES
  )

find_package(OpenMP)
if (OPENMP_FOUND)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()
