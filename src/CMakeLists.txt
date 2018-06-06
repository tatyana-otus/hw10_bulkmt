
set(BINARY_NAME bulkmt)

find_package(Threads)

add_executable(${BINARY_NAME} main.cpp )

target_link_libraries(${BINARY_NAME}
  ${CMAKE_THREAD_LIBS_INIT}
  )


target_include_directories(${BINARY_NAME} PRIVATE ../include)

set_target_properties(${BINARY_NAME} PROPERTIES
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
  COMPILE_OPTIONS -Wall -Wextra -Wpedantic 
)

#set (CMAKE_CXX_FLAGS "-fsanitize=thread")

install(TARGETS ${BINARY_NAME} RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT lisstic.tt@gmail.com)

include (CPack)