cmake_minimum_required(VERSION 2.6)

set(TLOC_PROJECT_NAME "game" CACHE STRING "The name of your project goes here")

set(internal_FIXME "FIXME!")

set(TLOC_PATH ${internal_FIXME} CACHE PATH "The root folder of this repository. You do not need to edit this manually.")
set(TLOC_DEP_PATH ${internal_FIXME} CACHE PATH "The tloc dependency repository path goes here. Make sure that you have run CMake on the repository.")

set(USER_PATH ${CMAKE_SOURCE_DIR} CACHE PATH "Root folder of this project.")
set(USER_LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/${ARCH_DIR} CACHE PATH "Default path for libraries in your project.")
set(USER_EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${ARCH_DIR} CACHE PATH "Default path for executables in your project.")
set(USER_EXECUTABLE_PROJECTS "main" CACHE STRING "Executable project names seperated by semicolons. All names must have their respective folders in the src/ folder with CMakeLists.txt file (see game/ folder for an example).")
set(USER_LIBRARY_PROJECTS "" CACHE STRING "Library project names seperated by semicolons. All names must have their respective folders in the src/ folder with CMakeLists.txt file (see game/ folder for an example).")

list(LENGTH USER_EXECUTABLE_PROJECTS tlocExecProjLength)
list(LENGTH USER_LIBRARY_PROJECTS tlocLibProjLength)
if(${tlocExecProjLength} EQUAL 0 AND 
   ${tlocLibProjLength} EQUAL 0)
   message(SEND_ERROR "You must have at least one executable or library")
endif()

if(${TLOC_PATH} MATCHES ${internal_FIXME})
    message(SEND_ERROR "Please enter a path for tlocEngine")
endif()
if(${TLOC_DEP_PATH} MATCHES ${internal_FIXME})
    message(SEND_ERROR "Please enter a path for tlocDep")
endif()

# -- For now, we force the user to use all libraries
#option(TLOC_INCLUDE_CORE        "Include tlocCore"     ON)
#option(TLOC_INCLUDE_GRAPHICS    "Include tlocGraphics" ON)
#option(TLOC_INCLUDE_INPUT       "Include tlocInput"    ON)
#option(TLOC_INCLUDE_MATH        "Include tlocMath"     ON)
#option(TLOC_INCLUDE_PHYSICS     "Include tlocPhysics"  ON)
#option(TLOC_INCLUDE_PREFAB      "Include tlocPrefab"   ON)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_DIR "x64")
else()
    set(ARCH_DIR "x86")
endif()

project(${TLOC_PROJECT_NAME})

add_subdirectory(src)

# Enable solution folders for MSVC
set_property(GLOBAL PROPERTY USE_FOLDERS ON)