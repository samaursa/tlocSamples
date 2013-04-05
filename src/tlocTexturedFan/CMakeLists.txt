set(SOURCE_FILES
  main.cpp
  ../tlocVersionCheck.cpp
  )

set(ASSETS
  ../../assets
  )

add_executable(${USER_CURRENT_EXEC_NAME}
  ${TLOC_APP_TYPE}
  ${SOURCE_FILES}
  ${ASSETS}
  )

add_common_libraries(${USER_CURRENT_EXEC_NAME})
