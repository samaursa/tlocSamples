set(SOURCE_FILES
  main.cpp
  )

add_executable(${USER_CURRENT_EXEC_NAME}
  ${SOURCE_FILES}
  )

target_link_libraries(${USER_CURRENT_EXEC_NAME}
  ${TLOC_PATH}/lib/${ARCH_DIR}/${CMAKE_CFG_INTDIR}/tlocCore.lib
  ${TLOC_PATH}/lib/${ARCH_DIR}/${CMAKE_CFG_INTDIR}/tlocGraphics.lib
  ${TLOC_PATH}/lib/${ARCH_DIR}/${CMAKE_CFG_INTDIR}/tlocMath.lib
  ${TLOC_PATH}/lib/${ARCH_DIR}/${CMAKE_CFG_INTDIR}/tlocInput.lib
  ${TLOC_PATH}/lib/${ARCH_DIR}/${CMAKE_CFG_INTDIR}/tlocPhysics.lib
  ${TLOC_PATH}/lib/${ARCH_DIR}/${CMAKE_CFG_INTDIR}/tlocPrefab.lib
  ${TLOC_DEP_PATH}/lib/${ARCH_DIR}/${CMAKE_CFG_INTDIR}/Box2D.lib
  )

if(MSVC)
  target_link_libraries(${USER_CURRENT_EXEC_NAME}
    glu32.lib
    OpenGL32.lib
    )
endif()

# Find dinput.lib and add it as a link target
if(WIN32)
  if(${ARCH_DIR} MATCHES "x86")
    find_library(DIRECT_INPUT_LIB
      NAMES dinput8.lib
      PATHS ${TLOC_DEP_PATH}/src/WinSDK/Win32/lib
      )
  else()
    find_library(DIRECT_INPUT_LIB
      NAMES dinput8.lib
      PATHS ${TLOC_DEP_PATH}/src/WinSDK/x64/lib
      )
  endif()

  target_link_libraries(${USER_CURRENT_EXEC_NAME}
    ${DIRECT_INPUT_LIB}
    )
endif()
