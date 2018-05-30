project(test)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)
find_package(Threads)

add_executable(test_main_cli test_main.cpp)
add_executable(test_log_cli  test_log.cpp)
add_executable(test_bulk_cli test_bulk.cpp)

target_include_directories(test_main_cli  PRIVATE ../include)
target_include_directories(test_main_cli  PRIVATE ${Boost_INCLUDE_DIR})

target_include_directories(test_log_cli PRIVATE ../include)
target_include_directories(test_log_cli PRIVATE ${Boost_INCLUDE_DIR})

target_include_directories(test_bulk_cli PRIVATE ../include)
target_include_directories(test_bulk_cli PRIVATE ${Boost_INCLUDE_DIR})

set_target_properties(test_main_cli test_log_cli test_bulk_cli PROPERTIES
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
  COMPILE_OPTIONS -Wpedantic -Wall -Wextra
  COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK
)
set (CMAKE_CXX_FLAGS "-fsanitize=thread")

target_link_libraries(test_main_cli
  ${CMAKE_THREAD_LIBS_INIT}	
  ${Boost_LIBRARIES}
  version
)

target_link_libraries(test_log_cli
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
)

target_link_libraries(test_bulk_cli
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
)

enable_testing()

add_test(test_main        test_main_cli -t test_suite_main)
add_test(test_bulk        test_bulk_cli -t test_suite_bulk)
add_test(test_log         test_log_cli  -t test_suite_log)