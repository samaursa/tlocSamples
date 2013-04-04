set(SOURCE_FILES
  main.cpp
  ../tlocVersionCheck.cpp
  )

add_executable(${USER_CURRENT_EXEC_NAME}
  ${SOURCE_FILES}
  )

add_common_libraries(${USER_CURRENT_EXEC_NAME})
