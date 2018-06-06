#include "lib.h"
#include <limits>

#define BOOST_TEST_MODULE test_main

#include "test_helper.h"


BOOST_AUTO_TEST_SUITE(test_suite_main)

BOOST_AUTO_TEST_CASE(test_version_valid)
{
    BOOST_CHECK( (major_version() >= 0) &&  (minor_version() >= 0) && (patch_version() >= 0) );
    BOOST_CHECK( (major_version() >  0) ||  (minor_version() >  0) || (patch_version() >  0) );
}


BOOST_AUTO_TEST_CASE(N_input)
{
    BOOST_CHECK_THROW(process("-1",  "0"), std::invalid_argument);
    BOOST_CHECK_THROW(process("0",   "0"), std::invalid_argument);
    BOOST_CHECK_THROW(process("0x10","0"), std::invalid_argument);
    BOOST_CHECK_THROW(process("a34", "0"), std::invalid_argument);

    auto str_1 = std::to_string(MAX_BULK_SIZE + 1);
    BOOST_CHECK_THROW(test_process(str_1.c_str(), "0", "", ""), std::invalid_argument);

    auto str_2 = std::to_string(MAX_BULK_SIZE);
    BOOST_CHECK_NO_THROW(test_process(str_2.c_str(), "0", "", "", false));
}


BOOST_AUTO_TEST_CASE(max_command_length)
{
    std::string in_data_1 (MAX_CMD_LENGTH, 'a');
    std::string out_data_1 =  "bulk: " + in_data_1 + "\n" ;
   
    BOOST_CHECK_NO_THROW(test_process("1",  "0", in_data_1, out_data_1, false));

    std::string in_data_2 (MAX_CMD_LENGTH + 1, 'a');    
    BOOST_CHECK_THROW(test_process("1",  "0", in_data_2, ""), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()