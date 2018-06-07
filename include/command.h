#include <iostream>
#include <algorithm>
#include <ctime>
#include <vector>
#include <memory>
#include <condition_variable>
#include <queue>
#include <thread>
#include <atomic>
#include <exception>
#include <sstream>

#include "queue_wrapper.h"

const size_t MAX_BULK_SIZE  = 128;
const size_t MAX_CMD_LENGTH = 64;

const size_t CIRCLE_BUFF_SIZE = 256;
static_assert(CIRCLE_BUFF_SIZE > 4, "");


using data_type = std::vector<std::string>;
using p_data_type = data_type*;
using queue_msg_type = std::queue<p_data_type>;

using p_print_task = std::shared_ptr<queue_wrapper<p_data_type>>;

using f_msg_type = std::tuple<p_data_type, std::time_t, size_t>;
using f_msg_type_ext   = std::tuple<f_msg_type, bool*>;
using p_file_tasks = std::shared_ptr<queue_wrapper<f_msg_type_ext>>;


struct Handler
{
    void stream_out(const p_data_type v, std::ostream& os)
    {   
        if(!v->empty()){
        
            ++blk_count;

            os << "bulk: " << *(v->cbegin());
            ++cmd_count;
            for (auto it = std::next(v->cbegin()); it != std::cend(*v); ++it){
                os << ", " << *it ;
                ++cmd_count;   
            }           
        }
        else {
            throw std::invalid_argument("Empty bulk !");
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

    std::atomic<bool> failure{false};
    std::exception_ptr eptr;
};


struct Command {

    Command(size_t N_, p_file_tasks f_task_,p_print_task print_task_ ):
    N(N_), file_task(f_task_), print_task(print_task_), braces_count(0),
    idx_write(0), file_id(0),
    str_count(0), cmd_count(0), blk_count(0)
    {
        for(size_t i = 0; i < CIRCLE_BUFF_SIZE; ++i) {

            data[i].reserve(MAX_BULK_SIZE);
            data_ext[i] = false;
        }
    }


    void start()
    {
        for (auto const& h : data_handler) {
            h->start();
        }
    }

    bool check_falure() const
    {
        for (auto const& h : data_handler) {
            if(h->failure) return true;
        }
        return false;
    }


    void handle_exeption() const
    {
        for (auto const& h : data_handler) {
            if (h->eptr) {
                std::rethrow_exception(h->eptr);
            }
        }
    }


    void stop()
    {
        for (auto const& h : data_handler) {
            h->quit = true;
        }

        std::unique_lock<std::mutex> lk(print_task->cv_mx);  // without lock -fsanitize=thread hung empty test 1:10 
        print_task->cv.notify_one();
        lk.unlock();

        std::unique_lock<std::mutex> lk_file(file_task->cv_mx);
        file_task->cv.notify_all();
        lk_file.unlock();

        for (auto const& h : data_handler) {
            h->stop();
        }

        handle_exeption();
    }


    void add_hanlder(std::shared_ptr<Handler> h)
    {
        data_handler.push_back(h);
    }


    void on_bulk_created()
    {
        if(!data[idx_write].empty()) {   

            data_ext[idx_write] = true;    //no need for lock

            set_logger_task();
            set_printer_task();

            update_write_index();
        }       
    }


    void set_logger_task()
    {
        std::unique_lock<std::mutex> lk_file(file_task->cv_mx);
        file_task->push(std::make_tuple(std::make_tuple(&data[idx_write], 
                                                       init_time, 
                                                       ++file_id
                                                       ), 
                                       &data_ext[idx_write]) 
                                       );
        lk_file.unlock();
        file_task->cv.notify_one();
    }


    void set_printer_task()
    {
        std::unique_lock<std::mutex> lk(print_task->cv_mx);
        

        if(print_task->size() > (CIRCLE_BUFF_SIZE - 3)) {
            print_task->cv.notify_one();
            print_task->cv_empty.wait(lk, [this](){ 
            return( this->print_task->size() <  (CIRCLE_BUFF_SIZE / 2)); });
        }
        
        print_task->push( &data[idx_write] );

        lk.unlock();

        print_task->cv.notify_one();
    }


    void update_write_index()
    {
        ++idx_write;
        idx_write %= CIRCLE_BUFF_SIZE;
        ++blk_count;

        std::unique_lock<std::mutex> lk_ext_data(file_task->cv_mx_empty);
        if(data_ext[idx_write] == true) { 
            file_task->cv.notify_one();
            file_task->cv_empty.wait(lk_ext_data, [this](){ return !this->data_ext[idx_write]; });
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


    void get_data(std::istream& is) {

        start();
        try {  
            for(std::string line; std::getline(is, line);){ 
                if(line.size() > MAX_CMD_LENGTH){
                    std::string msg = "Invalid command length. Command length must be < " 
                                      + std::to_string(MAX_CMD_LENGTH) + " :" + line + ".\n";
                    throw std::invalid_argument(msg.c_str());
                }
                if(check_falure()) throw std::runtime_error("");
                on_new_cmd(line);
            }
            on_cmd_end();
        }
        catch(const std::exception &e) {
            stop();
            throw;
        }
        stop();
    }


private:
    size_t N;
    p_file_tasks file_task;
    p_print_task print_task;

    std::time_t init_time;

    std::vector<std::shared_ptr<Handler>> data_handler;

    std::array<data_type, CIRCLE_BUFF_SIZE> data;
    std::array<bool,      CIRCLE_BUFF_SIZE> data_ext;
    
    int braces_count; 
    size_t idx_write;
    size_t file_id;

 public:   
    size_t str_count;
    size_t cmd_count;
    size_t blk_count;
};