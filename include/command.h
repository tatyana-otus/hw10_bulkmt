#include <iostream>
#include <algorithm>
#include <ctime>
#include <vector>
#include <memory>
#include <condition_variable>
#include <queue>
#include <thread>
#include <atomic>

const size_t MAX_BULK_SIZE  = 128;
const size_t MAX_CMD_LENGTH = 64;

const size_t CIRCLE_BUFF_SIZE = 256;
static_assert(CIRCLE_BUFF_SIZE > 4, "");

std::condition_variable cv;
std::mutex cv_m;

std::condition_variable cv_file;
std::mutex cv_m_file;

std::condition_variable cv_empty;

std::condition_variable cv_f_empty;
std::mutex cv_m_data_ext;

std::mutex std_err_mx;


using data_type = std::vector<std::string>;
using p_data_type = std::shared_ptr<data_type>;
using queue_msg_type = std::queue<p_data_type>;

using f_msg_type = std::tuple<p_data_type, std::time_t, size_t>;

using f_msg_type_ext   = std::tuple<f_msg_type, std::shared_ptr<bool>>;
using queue_f_msg_type = std::queue<f_msg_type_ext>;

queue_msg_type      msgs;
queue_f_msg_type    file_msgs;

size_t main_wait_count = 0;

struct Handler
{
    void stream_out(const p_data_type v, std::ostream& os)
    {   
        ++blk_count;

        os << "bulk" << ": " << *(v->cbegin());
        ++cmd_count;
        for (auto it = std::next(v->cbegin()); it != std::cend(*v); ++it){
            os << ", " << *it ;
            ++cmd_count;   
        } 
    }

    void print_metrics(std::ostream& os = std::cout) const
    {
        os << cmd_count << " commands, "
           << blk_count << " blocks" 
           <<  "\n";
    }

    size_t blk_count = 0;
    size_t cmd_count = 0;
    std::atomic<bool> quit{false};
};


struct Command {

    Command(size_t N_):N(N_), b_satic(true), cnt_braces(0)
    {
        for(auto& v : data) {
            v.reserve(MAX_BULK_SIZE);
        }

        for(auto& b : data_ext) {
            b = false;
        }      
    }


    void on_bulk_created()
    {
        if(!data[idx_write].empty()) {   


            std::unique_lock<std::mutex> lk_ext_data(cv_m_data_ext);
            data_ext[idx_write] = true;    
            lk_ext_data.unlock();

            std::unique_lock<std::mutex> lk_file(cv_m_file);
            f_msg_type file_msg = std::make_tuple(std::shared_ptr<data_type>(&data[idx_write], [](data_type*){}), init_time, ++file_id);
            file_msgs.push(std::make_tuple(file_msg, std::shared_ptr<bool>(&data_ext[idx_write], [](bool*){})));
            lk_file.unlock();

            cv_file.notify_one();

            //-------------------------------------
            std::unique_lock<std::mutex> lk(cv_m);
            if(msgs.size() > (CIRCLE_BUFF_SIZE - 3)) {
                cv.notify_one();
                cv_empty.wait(lk, [](){ return( msgs.size() <  (CIRCLE_BUFF_SIZE / 2)); });
            }
            msgs.push(std::shared_ptr<data_type>(&data[idx_write], [](data_type*){}));          
            lk.unlock();
            cv.notify_one();

            //-------
            ++idx_write;
            idx_write %= CIRCLE_BUFF_SIZE;
            ++blk_count;

            lk_ext_data.lock(); /////////////////////
            if(data_ext[idx_write] == true) { 
                ++main_wait_count;
                cv_file.notify_one();
                cv_f_empty.wait(lk_ext_data, [this](){ return !this->data_ext[idx_write]; });
            }
            lk_ext_data.unlock(); ///////////////////
        }       
    }

    
    enum class BulkState { end, save };

    void on_new_cmd(const std::string& d)
    {
        BulkState blk_state = BulkState::save;

        ++str_count;

        if(d == "{") {
            ++cnt_braces;
            if(b_satic == true){
                b_satic = false;
                blk_state = BulkState::end;
            }            
        }
        else if (d == "}"){
            --cnt_braces;
            if(cnt_braces == 0) {
                b_satic = true;
                blk_state = BulkState::end;
            }
            else if(cnt_braces < 0){
                throw std::invalid_argument("wrong command stream");
            }    
        }
        else {
            if(data[idx_write].empty())
                init_time = std::time(nullptr);
            data[idx_write].push_back(d);
            ++cmd_count;
        }

        exec_state(blk_state);
    }


    void on_cmd_end()
    {
        if(b_satic == true) {
            exec_state(BulkState::end);
        }
    }


    void exec_state(BulkState state) {

        switch(state) {

            case BulkState::end:
                on_bulk_created();
                data[idx_write].clear();
                break;

            case BulkState::save:
                if((b_satic == true) && (data[idx_write].size() == N)){
                    exec_state(BulkState::end);
                }
                break;
        }
    }


    void print_metrics(std::ostream& os = std::cout) const
    {
        os << cmd_count << " commands, "
           << blk_count << " blocks, " 
           << str_count << " strings"
           <<  "\n";
    }

    size_t str_count = 0;
    size_t cmd_count = 0;
    size_t blk_count = 0;

private:
    std::time_t init_time;
    std::array<std::vector<std::string>, CIRCLE_BUFF_SIZE> data;
    std::array<bool, CIRCLE_BUFF_SIZE> data_ext;

    size_t N;
    bool b_satic;
    int cnt_braces;  

    size_t idx_write = 0;

    size_t file_id = 0;
};