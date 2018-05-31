#include <chrono>
#include <thread>

#define BOOST_TEST_MODULE test_main

#include "test_helper.h"
#include <fstream>

BOOST_AUTO_TEST_SUITE(test_suite_metrics)


BOOST_AUTO_TEST_CASE(metrics_text)
{  
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
    "bulk: cmd4, cmd5, cmd6, cmd7\n\n"
    "Thread main   - 7 commands, 2 blocks, 9 strings\n"
    "Thread log    - 7 commands, 2 blocks\n"
    "Thread file_1 - 7 commands, 2 blocks\n";

    test_process("3", in_data, out_data, true, 1);


    std::string out_data_1 ="\n" 
    "Thread main   - 0 commands, 0 blocks, 0 strings\n"
    "Thread log    - 0 commands, 0 blocks\n"
    "Thread file_1 - 0 commands, 0 blocks\n"
    "Thread file_2 - 0 commands, 0 blocks\n"
    "Thread file_3 - 0 commands, 0 blocks\n"
    "Thread file_4 - 0 commands, 0 blocks\n";

    test_process("3", "", out_data_1, true, 4);
}

BOOST_AUTO_TEST_CASE(metrics_count)
{
    std::stringstream iss;
    std::stringstream oss;
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

    iss << in_data;
    process("3", iss, oss, ess, true, 1);
    check_metrics(9, 7, 2);

    iss.clear();
    iss << in_data;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    process("3", iss, oss, ess, true, 2);
    check_metrics(9, 7, 2);

    iss.clear();
    iss << in_data;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    process("3", iss, oss, ess, true, 4);
    check_metrics(9, 7, 2);
}

BOOST_AUTO_TEST_SUITE_END()