set(SOURCE_FILES
  main.cpp
  ../tlocVersionCheck.cpp
  )

set(ASSETS_PATH
  ../../assets)

if (${USER_CURRENT_PROJECT_TYPE} STREQUAL ${USER_PROJECT_TYPE_EXE})

  add_executable(${USER_CURRENT_PROJECT_NAME}
    ${TLOC_APP_TYPE}
    ${SOURCE_FILES}
    ${ASSETS_PATH}
    )
  add_common_libraries(${USER_CURRENT_PROJECT_NAME})
  if (ASSETS_PATH)
    add_apple_plist(${USER_CURRENT_PROJECT_NAME} ${ASSETS_PATH})
  endif()

elseif(${USER_CURRENT_PROJECT_NAME} STREQUAL ${USER_PROJECT_TYPE_LIB})

  add_library(${USER_CURRENT_PROJECT_NAME} STATIC
    ${SOURCE_FILES}
    )

endif()

