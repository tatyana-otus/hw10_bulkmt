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

const size_t MAX_QUEUE_SIZE = 1024;

using data_type = std::vector<std::string>;

using p_data_type = std::shared_ptr<data_type>;
using p_tasks_t = queue_wrapper<std::shared_ptr<data_type>>;

using f_msg_type = std::tuple<p_data_type, std::time_t, size_t>;
using f_tasks_t = queue_wrapper<f_msg_type>;


struct Handler
{
    void stream_out(const p_data_type v, std::ostream& os)
    {   
        if(!v->empty()){
        
            ++blk_count;

            os << "bulk: " << *(v->cbegin());
            cmd_count += v->size();
            for (auto it = std::next(v->cbegin()); it != std::cend(*v); ++it){
                os << ", " << *it ;   
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

    Command(size_t N_, std::shared_ptr<f_tasks_t> f_task_, std::shared_ptr<p_tasks_t> print_task_ ):
    N(N_), file_task(f_task_), print_task(print_task_), braces_count(0),
    file_id(0),
    str_count(0), cmd_count(0), blk_count(0)
    {
        cur_data = std::make_shared<data_type>();
        cur_data->reserve(N);
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

        print_task->cv.notify_one();
        file_task->cv.notify_all();

        for (auto const& h : data_handler) {
            h->stop();
        }

        handle_exeption();
    }


    void add_hanlder(std::shared_ptr<Handler> h)
    {
        data_handler.push_back(h);
    }

private:
    void on_bulk_created()
    { 
        if(!cur_data->empty()) { 

            set_logger_task();
            set_printer_task();

            update_write_index();
        }       
    }


    void set_logger_task()
    {
        std::unique_lock<std::mutex> lk_file(file_task->cv_mx);

        if(file_task->size() > MAX_QUEUE_SIZE) {
            file_task->cv.notify_one();
            file_task->cv_empty.wait(lk_file, [this](){ 
            return( this->file_task->size() <  MAX_QUEUE_SIZE); });
        }

        file_task->push(std::make_tuple(cur_data, init_time, ++file_id));
        lk_file.unlock();
        file_task->cv.notify_one();
    }


    void set_printer_task()
    {
        std::unique_lock<std::mutex> lk(print_task->cv_mx);
        
        if(print_task->size() > MAX_QUEUE_SIZE) {
            print_task->cv.notify_one();
            print_task->cv_empty.wait(lk, [this](){ 
            return( this->print_task->size() <  MAX_QUEUE_SIZE); });
        }
        
        print_task->push( cur_data );
        lk.unlock();
        print_task->cv.notify_one();
    }


    void update_write_index()
    {
        ++blk_count;

        cur_data = nullptr;
        cur_data = std::make_shared<data_type>();
        cur_data->reserve(N);
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
            if(cur_data->empty())
                init_time = std::time(nullptr);
            cur_data->push_back(d);
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
                break;

            case BulkState::save:
                if((braces_count == 0) && (cur_data->size() == N)){
                    exec_state(BulkState::end);
                }
                break;
        }
    }

public:
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

    std::shared_ptr<f_tasks_t> file_task;
    std::shared_ptr<p_tasks_t> print_task;

    std::time_t init_time;

    std::vector<std::shared_ptr<Handler>> data_handler;
    
    int braces_count; 
    size_t file_id;

    std::shared_ptr<data_type> cur_data;  

 public:   
    size_t str_count;
    size_t cmd_count;
    size_t blk_count;
};