# -----------------------------------------------------------------------------
# Functions

function(add_common_libraries EXEC_NAME)
  target_link_libraries(${EXEC_NAME}
    tlocCore
    tlocGraphics
    tlocMath
    tlocInput
    tlocPhysics
    tlocPrefab
    Box2D
    )

  if (TLOC_PLATFORM_WIN32)

    # Add OpenGL libraries for Windows
    target_link_libraries(${EXEC_NAME}
      glu32.lib
      OpenGL32.lib
      )

    # Find dinput.lib and add it as a link target
    if(${ARCH_DIR} MATCHES "x86")
      find_library(DIRECT_INPUT_LIB
        NAMES dinput8.lib
        PATHS ${TLOC_DEP_PATH}/src/WinSDK/WinSDK/Win32/lib
        )
    else()
      find_library(DIRECT_INPUT_LIB
        NAMES dinput8.lib
        PATHS ${TLOC_DEP_PATH}/src/WinSDK/WinSDK/x64/lib
        )
    endif()

    target_link_libraries(${EXEC_NAME}
      ${DIRECT_INPUT_LIB}
      )
  endif()
endfunction()

function(add_apple_plist EXEC_NAME ASSETS_PATH)
  if(TLOC_PLATFORM_IOS)
    set(EXECUTABLE_NAME "\${EXECUTABLE_NAME}" PARENT_SCOPE)
    set(PRODUCT_NAME    "\${PRODUCT_NAME}" PARENT_SCOPE)
    set(BUNDLE_NAME     ${EXEC_NAME} PARENT_SCOPE)
    set_target_properties(${EXEC_NAME} PROPERTIES 
		MACOSX_BUNDLE_INFO_PLIST ${TLOC_PATH}/platform/iOS/tlocEngine.plist)
    set_target_properties(${EXEC_NAME} PROPERTIES
    		RESOURCE "${ASSETS_PATH}")
  endif()
endfunction()

# -----------------------------------------------------------------------------

# Remove MinSizeRel
set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo CACHE TYPE INTERNAL FORCE)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# Define TLOC_CXX03 if C++11 is NOT supported (force XCode to C++03 for now)
if(NOT TLOC_COMPILER_C11 OR TLOC_COMPILER_XCODE)
  add_definitions(-DTLOC_CXX03)
endif()

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

if(TLOC_COMPILER_MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG           "/Od /Gm /RTC1 /MTd /GR- /W4 /WX /c /Zi /TP")
  set(CMAKE_CXX_FLAGS_RELEASE         "/O2 /Ob2 /Oi /Ot /GL /MT /Gy /GR- /W4 /WX /c /Zi /TP")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "/O2 /Ob2 /Oi /Ot /Gm /MT /Gy /GR- /W4 /WX /c /Zi /TP")

  #turn off exceptions for all configurations
  string(REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE "/Zm1000" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  string(REGEX REPLACE "/GX" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
elseif(TLOC_COMPILER_XCODE)
  set(CMAKE_CXX_FLAGS "-x objective-c++")
  set(CMAKE_CXX_FLAGS_DEBUG "-fno-exceptions -fno-rtti -Wno-unused-function")
  set(CMAKE_CXX_FLAGS_RELEASE "-fno-exceptions -fno-rtti -Wno-unused-function")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-fno-exceptions -fno-rtti -Wno-unused-function")
  set(CMAKE_EXE_LINKER_FLAGS "-framework Foundation -framework OpenGLES -framework AudioToolbox -framework CoreGraphics -framework QuartzCore -framework UIKit -framework OpenAL")
else()
  message("Configuration not applied because compiler is unsupported (or not detected properly)")
endif()

# common includes
include_directories(
    ${TLOC_PATH}/src/
    ${TLOC_DEP_PATH}/include/
    ${TLOC_DEP_PATH}/include/Box2D/
    ${USER_PATH}/src/

    ${CMAKE_SOURCE_DIR}/src/
    )

# common link directories
link_directories(
  ${TLOC_BUILD_PATH}/src/tlocCore/
  ${TLOC_BUILD_PATH}/src/tlocGraphics/
  ${TLOC_BUILD_PATH}/src/tlocMath/
  ${TLOC_BUILD_PATH}/src/tlocInput/
  ${TLOC_BUILD_PATH}/src/tlocPhysics/
  ${TLOC_BUILD_PATH}/src/tlocPrefab/

  ${TLOC_DEP_BUILD_PATH}/src/Box2D/
  )

# -----------------------------------------------------------------------------
# Add all libraries and executables

message("")
message(STATUS "-- Executables --")
foreach(exec ${USER_EXECUTABLE_PROJECTS})
  message(STATUS "Adding ${exec} executable project")
  set(USER_CURRENT_EXEC_NAME ${exec})
  add_subdirectory(${exec})
endforeach()
message(STATUS "-- Executables -- DONE")

message("")
message(STATUS "-- Libraries --")
foreach(lib ${USER_LIBRARY_PROJECTS})
  message(STATUS "Adding ${lib} library project")
  add_subdirectory(${lib})
endforeach()
message(STATUS "-- Libraries -- DONE") 

# -----------------------------------------------------------------------------
# Generate tlocAssetPath.h file
FILE(WRITE ${USER_ASSETS_PATH} "
#pragma once

#ifndef _ASSETS_PATH_H_
#define _ASSETS_PATH_H_

#include <cstring>

#if defined (__APPLE__) && defined (__OBJC__)
# import <Foundation/Foundation.h>
#endif

#define TLOC_ASSETS_PATH \"${CMAKE_SOURCE_DIR}/assets/\"

#if defined(TLOC_OS_WIN)

const char* GetAssetPath()
{
  static const char* assetPath = \"${CMAKE_SOURCE_DIR}/assets/\";
  return assetPath;
}
#elif defined(TLOC_OS_IPHONE)
const char* GetAssetPath()
{
  static char assetPath[1024];
  strcpy(assetPath, [[[NSBundle mainBundle] resourcePath]
                     cStringUsingEncoding:[NSString defaultCStringEncoding]]);
  strcat(assetPath, \"/assets/\");

  return assetPath;
}
#endif

#endif // _ASSETS_PATH_H_

")

