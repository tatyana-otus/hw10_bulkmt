#include <chrono>
#include <thread>

#define BOOST_TEST_MODULE test_main

#include "test_helper.h"
#include <fstream>

BOOST_AUTO_TEST_SUITE(test_suite_log)


BOOST_AUTO_TEST_CASE(log_file_creation)
{  
    std::stringstream ess;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::string  in_data =  
    "cmd1\n"
    "cmd2\n"
    "cmd3\n"
    "{\n"
    "cmd4\n"
    "cmd5\n"
    "cmd6\n"
    "cmd7\n"
    "}\n";

    std::string out_data =
    "bulk: cmd1, cmd2, cmd3\n"
    "bulk: cmd4, cmd5, cmd6, cmd7\n";

    std::string bulk_1 = "bulk: cmd1, cmd2, cmd3";
    std::string bulk_2 = "bulk: cmd4, cmd5, cmd6, cmd7";

    std::time_t start_time = std::time(nullptr);
    ess = test_process("3", in_data, out_data, false, 2);
    BOOST_CHECK_EQUAL( ess.str(), "" );

    std::string f_name_1 = "bulk" + std::to_string(start_time) + "_1.log";
    std::string f_name_2 = "bulk" + std::to_string(start_time) + "_2.log"; 

    std::ifstream file_1{f_name_1};
    std::ifstream file_2{f_name_2};

    BOOST_CHECK_EQUAL( file_1.good(), true );
    BOOST_CHECK_EQUAL( file_2.good(), true );

    std::stringstream buff_1;
    std::stringstream buff_2;
    buff_1 << file_1.rdbuf();
    buff_2 << file_2.rdbuf();

    BOOST_CHECK_EQUAL( buff_1.str(), bulk_1 );
    BOOST_CHECK_EQUAL( buff_2.str(), bulk_2 );

    file_1.close();
    file_2.close();
}

BOOST_AUTO_TEST_SUITE_END()