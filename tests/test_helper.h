#include <sstream>
#include <numeric>
#include <boost/test/unit_test.hpp>
#include <boost/test/execution_monitor.hpp> 
#include <boost/test/output_test_stream.hpp>
#include "functions.h"

using boost::test_tools::output_test_stream;


void test_process(const char* N, std::string th_id,
                               const std::string& in_data, 
                               const std::string& out_data,                            
                               bool is_metrics = true, 
                               size_t file_th_cnt = 2)
{
    std::stringstream iss;
    std::stringstream oss;

    iss << in_data;
    process(N, th_id, iss, oss, is_metrics, file_th_cnt);

    BOOST_CHECK_EQUAL( oss.str(), out_data);
}


void check_metrics(size_t str_count, size_t cmd_count, size_t blk_count, 
                   unsigned long long N,
                   std::stringstream& iss, 
                   std::stringstream& oss, 
                   size_t file_th_cnt,
                   std::string id)
{
    auto  q_file  = std::make_shared<queue_wrapper<f_msg_type_ext>>();
    auto  q_print = std::make_shared<queue_wrapper<p_data_type>>();

    auto cmd = std::make_shared<Command>(N, q_file, q_print);

    auto data_log = std::make_shared<PrintData>(q_print, oss); 

    std::vector<std::shared_ptr<WriteData>> file_log;
    file_log.resize(file_th_cnt);
    for(size_t i = 0; i < file_th_cnt; ++i){
        file_log[i] = std::make_shared<WriteData>(q_file, id);
    }


    cmd->add_hanlder(data_log);
    for(auto& h : file_log){
        cmd->add_hanlder(h);
    }

    cmd->get_data(iss);

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