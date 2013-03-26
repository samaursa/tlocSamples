# Remove MinSizeRel
set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo CACHE TYPE INTERNAL FORCE)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# Common pre-processors
if(CMAKE_CXX_FLAGS_DEBUG)
    add_definitions(-DTLOC_DEBUG)
    add_definitions(-DDEBUG)
elseif(CMAKE_CXX_FLAGS_RELEASE)
    add_definitions(-DTLOC_RELEASE)
    add_definitions(-DNDEBUG)
elseif(CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    add_definitions(-DTLOC_RELEASE_DEBUGINFO)
endif()

# Backup the default flags
SET(CMAKE_CXX_FLAGS_DEFAULT                 ${CMAKE_CXX_FLAGS})
set(CMAKE_CXX_FLAGS_DEBUG_DEFAULT           ${CMAKE_CXX_FLAGS_DEBUG})
set(CMAKE_CXX_FLAGS_RELEASE_DEFAULT         ${CMAKE_CXX_FLAGS_RELEASE})
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_DEFAULT  ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})

if(MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG           "/Od /Gm /RTC1 /MTd /GR- /W4 /WX /c /Zi /TP")
  set(CMAKE_CXX_FLAGS_RELEASE         "/O2 /Ob2 /Oi /Ot /GL /MT /Gy /GR- /W4 /WX /c /Zi /TP")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "/O2 /Ob2 /Oi /Ot /Gm /MT /Gy /GR- /W4 /WX /c /Zi /TP")

  #turn off exceptions for all configurations
  string(REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

# lib/exe path for all configurations 
set(LIBRARY_OUTPUT_PATH    ${USER_LIBRARY_OUTPUT_PATH}/lib/${ARCH_DIR})
set(EXECUTABLE_OUTPUT_PATH ${USER_EXECUTABLE_OUTPUT_PATH}/bin/${ARCH_DIR})

# common includes
include_directories(
    ${TLOC_PATH}/src/
    ${TLOC_DEP_PATH}/include/
    ${TLOC_DEP_PATH}/include/Box2D/
    ${USER_PATH}/src/
    )

foreach(exec ${USER_EXECUTABLE_PROJECTS})
    message("Adding ${exec} executable project")
    set(USER_CURRENT_EXEC_NAME ${exec})
    add_subdirectory(${exec})
endforeach()

foreach(lib ${USER_LIBRARY_PROJECTS})
    message("Adding ${lib} library project")
    add_subdirectory(${lib})
endforeach()