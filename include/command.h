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

std::condition_variable cv_data_ext;
std::mutex cv_m_data_ext;

std::mutex std_err_mx;


using data_type = std::vector<std::string>;
//using p_data_type = std::shared_ptr<data_type>;
using p_data_type = data_type*;
using queue_msg_type = std::queue<p_data_type>;

using f_msg_type = std::tuple<p_data_type, std::time_t, size_t>;
//using f_msg_type_ext   = std::tuple<f_msg_type, std::shared_ptr<bool>>;
using f_msg_type_ext   = std::tuple<f_msg_type, bool*>;
using queue_f_msg_type = std::queue<f_msg_type_ext>;

queue_msg_type      msgs;
queue_f_msg_type    file_msgs;


struct Handler
{
    void stream_out(const p_data_type v, std::ostream& os)
    {   
        ++blk_count;

        os << "bulk: " << *(v->cbegin());
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

    void stop (void) 
    {
        helper_thread.join();
    } 

    virtual void start(void) = 0; 


    std::thread helper_thread;

    size_t blk_count = 0;
    size_t cmd_count = 0;
    std::atomic<bool> quit{false};
};


struct Command {

    Command(size_t N_):N(N_), braces_count(0)
    {
        for(size_t i = 0; i < CIRCLE_BUFF_SIZE; ++i) {

            data[i].reserve(MAX_BULK_SIZE);
            data_ext[i] = false;

            //data_shared_ptr[i]     = std::shared_ptr<data_type>(&data[i],     [](data_type*){});
            //data_ext_shared_ptr[i] = std::shared_ptr<bool>     (&data_ext[i], [](bool*){});
        }
    }


    void start()
    {
        for (auto const& h : data_handler) {
            h->start();
        }
    }


    void stop()
    {
        for (auto const& h : data_handler) {
            h->quit = true;
        }

        std::unique_lock<std::mutex> lk(cv_m);  // without lock -fsanitize=thread hung empty test 1:10 
        cv.notify_one();
        lk.unlock();
        
        std::unique_lock<std::mutex> lk_file(cv_m_file); // without lock -fsanitize=thread hung empty test 1:10
        cv_file.notify_all();
        lk_file.unlock();

        for (auto const& h : data_handler) {
            h->stop();
        }
    }


    void add_hanlder(std::shared_ptr<Handler> h)
    {
        data_handler.push_back(h);
    }


    void on_bulk_created()
    {
        if(!data[idx_write].empty()) {   

            data_ext[idx_write] = true;    //no need for lock

            set_printer_task();
            set_logger_task();

            update_write_index();
        }       
    }


    void set_printer_task()
    {
        std::unique_lock<std::mutex> lk_file(cv_m_file);

        file_msgs.push(std::make_tuple(std::make_tuple(&data[idx_write], //data_shared_ptr[idx_write]
                                                       init_time, 
                                                       ++file_id
                                                       ), 
                                       &data_ext[idx_write]) // data_ext_shared_ptr[idx_write]
                                       );                                                       
       
        lk_file.unlock();

        cv_file.notify_one();
    }


    void set_logger_task()
    {
        std::unique_lock<std::mutex> lk(cv_m);

        if(msgs.size() > (CIRCLE_BUFF_SIZE - 3)) {
            cv.notify_one();
            cv_empty.wait(lk, [](){ return( msgs.size() <  (CIRCLE_BUFF_SIZE / 2)); });
        }
        
        msgs.push( &data[idx_write] );  //data_shared_ptr[idx_write] 

        lk.unlock();

        cv.notify_one();
    }


    void update_write_index()
    {
        ++idx_write;
        idx_write %= CIRCLE_BUFF_SIZE;
        ++blk_count;

        std::unique_lock<std::mutex> lk_ext_data(cv_m_data_ext);
        if(data_ext[idx_write] == true) { 
            cv_file.notify_one();
            cv_data_ext.wait(lk_ext_data, [this](){ return !this->data_ext[idx_write]; });
        }
        lk_ext_data.unlock(); 
    }

    
    enum class BulkState { end, save };

    void on_new_cmd(const std::string& d)
    {
        BulkState blk_state = BulkState::save;

        ++str_count;

        if(d == "{") {  
            if(braces_count == 0) {
                blk_state = BulkState::end;
            }
            ++braces_count;        
        }
        else if (d == "}"){
            --braces_count;
            if(braces_count == 0) {
                blk_state = BulkState::end;
            }
            else if(braces_count < 0){
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
        if(braces_count == 0){
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
                if((braces_count == 0) && (data[idx_write].size() == N)){
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

    std::vector<std::shared_ptr<Handler>> data_handler;

    std::array<data_type,             CIRCLE_BUFF_SIZE> data;
    //std::array<p_data_type,           CIRCLE_BUFF_SIZE> data_shared_ptr;

    std::array<bool,                  CIRCLE_BUFF_SIZE> data_ext;
    //std::array<std::shared_ptr<bool>, CIRCLE_BUFF_SIZE> data_ext_shared_ptr;

    size_t N;
    int braces_count; 
    size_t idx_write = 0;
    size_t file_id = 0;
};