#include <sstream>
#include <numeric>
#include <boost/test/unit_test.hpp>
#include <boost/test/execution_monitor.hpp> 
#include <boost/test/output_test_stream.hpp>
#include "functions.h"

using boost::test_tools::output_test_stream;

std::stringstream test_process(const char* N, 
                               const std::string& in_data, 
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


void check_metrics(size_t str_count, size_t cmd_count, size_t blk_count)
{
    BOOST_CHECK_EQUAL( cmd->str_count, str_count);
    BOOST_CHECK_EQUAL( cmd->cmd_count, cmd_count);
    BOOST_CHECK_EQUAL( cmd->blk_count, blk_count);

    BOOST_CHECK_EQUAL( cmd->cmd_count, data_log->cmd_count);
    BOOST_CHECK_EQUAL( cmd->blk_count, data_log->blk_count);

    BOOST_CHECK_EQUAL( cmd->cmd_count, 
                      std::accumulate(file_log.begin(), file_log.end(), 0, 
                                      [](int sum, auto p) {
                                        return sum + p->cmd_count;
                                       }));
    BOOST_CHECK_EQUAL( cmd->blk_count, 
                      std::accumulate(file_log.begin(), file_log.end(), 0, 
                                     [](int sum, auto p) {
                                        return sum + p->blk_count;
                                      }));
}