#include <sstream>
#include <boost/test/unit_test.hpp>
#include <boost/test/execution_monitor.hpp> 
#include <boost/test/output_test_stream.hpp>
#include "functions.h"

using boost::test_tools::output_test_stream;

std::stringstream test_process(const char* N, const std::string& in_data, 
                                 const std::string& out_data,                            
                                 bool is_metrics = true, 
                                 size_t file_th_cnt = 2)
{
    std::stringstream iss;
    std::stringstream oss;
    std::stringstream ess;

    iss << in_data;
    process(N, iss, oss, ess, is_metrics, file_th_cnt);

    BOOST_CHECK_EQUAL( oss.str(), out_data);

    return ess;
}


/*void one_cmd1_pass(std::stringstream& err_stream)
{
    std::stringstream iss;
    std::stringstream oss;

    iss << "cmd1\n";
    process("1", iss, oss, err_stream);
}*/