#include "command.h"
#include <fstream>


struct WriteData : public Handler
{
    WriteData(p_file_tasks task_, std::string process_id_):process_id(process_id_), task(task_) {}

    void start(void) override
    {
        helper_thread = std::thread(&WriteData::on_bulk_resolved, 
                                    this);
    } 


    void create_bulk_file(f_msg_type& msg) 
    {
        p_data_type v;
        std::time_t t;
        size_t id;
        std::tie(v, t, id) = msg;

        std::string file_name = "bulk" + std::to_string(t)  + "_" 
                                       + process_id + "_"
                                       + std::to_string(id) + ".log";
        
        if(!std::ifstream{file_name}){

            std::ofstream of{file_name};
            if(of.good() != true){
                std::string msg = "Can't create file: " + file_name + '\n';
                throw std::runtime_error(msg.c_str());
            }

            stream_out(v, of);

            of.close();
        }
        else {
            std::string msg = file_name + " log file already exists\n";
            throw std::runtime_error(msg.c_str());
        }  
    }


    void on_bulk_resolved()   
    {
        try{
            while(!quit){
                std::unique_lock<std::mutex> lk_file(task->cv_mx);
                task->cv.wait(lk_file, [this](){ return (!this->task->empty() || this->quit); });

                if(task->empty()) break;

                auto m_ex = task->front();

                f_msg_type m;
                bool* b;

                std::tie(m, b) = m_ex;
                task->pop();          
                lk_file.unlock();

                create_bulk_file(m);

                std::lock_guard<std::mutex> lk_ext_data(task->cv_mx_empty);
                *b = false;

                task->cv_empty.notify_one(); 
            } 

            std::unique_lock<std::mutex> lk_file(task->cv_mx);
            while(!task->empty()) {
                auto m_ex = task->front();

                f_msg_type m;
                bool* b;
                std::tie(m, b) = m_ex;
                task->pop();
                create_bulk_file(m);

                std::lock_guard<std::mutex> lk_ext_data(task->cv_mx_empty);
                *b = false;  
                task->cv_empty.notify_one(); 
            }
        }
        catch(const std::exception &e) {
            eptr = std::current_exception();
            failure = true;           
        }     
    }

        std::string process_id;
    private:
        p_file_tasks task;
        
};


struct PrintData : public Handler
{
    PrintData(p_print_task task_, std::ostream& os_ = std::cout):task(task_), os(os_) {}

    void start(void) override
    {
        helper_thread = std::thread( &PrintData::on_bulk_resolved,
                                     this);
    } 


    void on_bulk_resolved() 
    {
        try {
            while(!quit){
                std::unique_lock<std::mutex> lk(task->cv_mx);

                task->cv.wait(lk, [this](){ return (!this->task->empty() || this->quit); });

                if(task->empty()) break;

                auto v = task->front();
                task->pop();
                if (task->size() <  (CIRCLE_BUFF_SIZE / 2)) task->cv_empty.notify_one();
                lk.unlock();
                
                stream_out(v, os);
                os << "\n";
            } 

            std::unique_lock<std::mutex> lk(task->cv_mx);
            while(!task->empty()) {
                auto v = task->front();
                task->pop();
                stream_out(v, os);
                os << '\n';
            }
        }
        catch(const std::exception &e) {
            eptr = std::current_exception();
            failure = true;           
        }    
    }

private:
    p_print_task task;
    std::ostream& os;   
};