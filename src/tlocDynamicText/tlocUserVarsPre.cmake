#------------------------------------------------------------------------------
# This file is included AFTER CMake adds the executable/library
# Do NOT remove the following variables. Modify the variables to suit your 
# project.

# Do NOT remove the following variables. Modify the variables to suit your project
set(USER_SOURCE_FILES
  main.cpp
  )

# Do not include individual assets here. Only add paths
set(USER_ASSETS_PATH
  ../../assets
  )

# Dependent project is compiled after dependency
set(USER_PROJECT_DEPENDENCIES
  )

# Libraries that the executable needs to link against
set(USER_EXECUTABLE_LINK_LIBRARIES
  freetype.lib
  )

link_directories(
  ${TLOC_DEP_BUILD_PATH}/src/FreeType/
  )

include_directories(
  ${TLOC_DEP_PATH}/include/FreeType/
  )
