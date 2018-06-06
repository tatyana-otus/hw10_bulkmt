
#define BOOST_TEST_MODULE test_main

#include "test_helper.h"

size_t test_id = 100;

BOOST_AUTO_TEST_SUITE(test_suite_bulk)



BOOST_AUTO_TEST_CASE(empty_bulk)
{  
    test_process("2", std::to_string(++test_id), "", "", false);
}

BOOST_AUTO_TEST_CASE(static_bulk)
{  
    std::string  in_data = 
    "cmd1\n"
    "cmd2\n"
    "cmd3\n"
    "cmd4\n"
    "cmd5\n"; 

    std::string out_data =
    "bulk: cmd1, cmd2, cmd3\n"
    "bulk: cmd4, cmd5\n";

    test_process("3", std::to_string(++test_id), in_data, out_data, false, 2);
}


BOOST_AUTO_TEST_CASE(dinamic_bulk)
{
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

    test_process("3", std::to_string(++test_id), in_data, out_data, false, 2);
}


BOOST_AUTO_TEST_CASE(dinamic_enclosed_bulk)
{ 
    std::string  in_data = 
    "{\n"
    "cmd1\n"
    "cmd2\n"
    "{\n"
    "cmd3\n"
    "cmd4\n"
    "}\n"
    "cmd5\n"
    "cmd6\n"
    "}\n";  

    std::string out_data =
    "bulk: cmd1, cmd2, cmd3, cmd4, cmd5, cmd6\n";

    test_process("3", std::to_string(++test_id), in_data, out_data, false, 2);
}


BOOST_AUTO_TEST_CASE(incomplete_dinamic_bulk)
{
    std::string  in_data = 
        "cmd1\n"
        "cmd2\n"
        "cmd3\n"
        "{\n"
        "cmd4\n"
        "cmd5\n"
        "cmd6\n"
        "cmd7\n";

    std::string out_data =
    "bulk: cmd1, cmd2, cmd3\n";

    test_process("3", std::to_string(++test_id), in_data, out_data, false, 2);
}


BOOST_AUTO_TEST_CASE(cmd_stream)
{
    std::string  in_data = 
        "cmd1\n"
        "cmd2\n"
        "{cmd3\n"
        "\n"
        "\n"
        "} \n"
        "{\n"
        "cmd4\n"
        "{\n"
        "cmd5\n"
        "}\n"
        "}\n"
        "cmd6\n"
        "cmd7./{}[]¯\\_ (ツ) _/¯\n"
        "cmd8";

    std::string out_data =
    "bulk: cmd1, cmd2\n"
    "bulk: {cmd3, \n"
    "bulk: , } \n"
    "bulk: cmd4, cmd5\n"
    "bulk: cmd6, cmd7./{}[]¯\\_ (ツ) _/¯\n"
    "bulk: cmd8\n";

    test_process("2", std::to_string(++test_id), in_data, out_data, false, 2);
}


BOOST_AUTO_TEST_CASE(wrong_cmd_stream)
{
    std::string  in_data = 
        "}\n"
        "cmd1\n"
        "cmd2\n"
        "cmd3\n"
        "{\n";

    std::string out_data = "";    

    BOOST_CHECK_THROW(test_process("2", std::to_string(++test_id), 
                                   in_data, out_data), 
                                   std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(cmd_break)
{  
    test_process("2", std::to_string(++test_id), "cmd1\n", "bulk: cmd1\n", false, 2);
    test_process("2", std::to_string(++test_id), "cmd",    "bulk: cmd\n",  false, 2);
}

BOOST_AUTO_TEST_SUITE_END()