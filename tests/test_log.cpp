#include <chrono>
#include <thread>

#define BOOST_TEST_MODULE test_main

#include "test_helper.h"
#include <fstream>

size_t test_id = 200;

BOOST_AUTO_TEST_SUITE(test_suite_log)


BOOST_AUTO_TEST_CASE(exep_log_file_already_exists)
{  
    std::string  in_data =  
    "cmd1\n"
    "cmd2\n"
    "cmd3\n";

    std::stringstream iss;
    std::stringstream oss;

    iss << in_data;

    std::time_t start_time = std::time(nullptr);
    
    int count = 10;
    ++test_id;
    while(--count) {
        std::string f_name = "bulk" + std::to_string(start_time++) +
                                    + "_" + std::to_string(test_id) 
                                    + "_1.log";
        std::ofstream of{f_name};
        BOOST_CHECK_EQUAL( of.good(), true );
        of.close();
    }    

    BOOST_CHECK_THROW(process("3", std::to_string(test_id), 
                              iss, oss, false, 2
                              ), 
                      std::logic_error
                      ); 
}


BOOST_AUTO_TEST_CASE(exep_can_not_create_log_file)
{  
    std::string  in_data =  
    "cmd1\n"
    "cmd2\n"
    "cmd3\n";

    std::stringstream iss;
    std::stringstream oss;

    iss << in_data;

    std::time_t start_time = std::time(nullptr);
    

    BOOST_CHECK_THROW(process("3", "//", 
                              iss, oss, false, 2
                              ), 
                      std::runtime_error
                      ); 
}

BOOST_AUTO_TEST_SUITE_END()