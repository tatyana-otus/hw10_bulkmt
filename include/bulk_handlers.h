#include "command.h"
#include <fstream>


struct WriteData : public Handler
{
    WriteData(std::shared_ptr<f_tasks_t> task_, std::string main_th_id_):main_th_id(main_th_id_), task(task_) {}

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
                                       + main_th_id + "_"
                                       + std::to_string(id) + ".log";
        
        if(!std::ifstream{file_name}){

            std::ofstream of{file_name};
            if(of.good() != true){
                std::string msg = "Can't create file: " + file_name + '\n';
                throw std::runtime_error(msg.c_str());
            }

            stream_out(v, of);

            of.close();
            if(!of.good()) {
                throw std::runtime_error("Error writing to file.");
            } 
        }
        else {
            std::string msg = file_name + " log file already exists\n";
            throw std::logic_error(msg.c_str());
        }  
    }


    void on_bulk_resolved()   
    {
        try{
            while(!quit){
                std::unique_lock<std::mutex> lk_file(task->cv_mx);
                task->cv.wait(lk_file, [this](){ return (!this->task->empty() || this->quit); });

                if(task->empty()) break;

                auto m = task->front();

                task->pop(); 
                if (task->size() <  (MAX_QUEUE_SIZE / 2)) task->cv_empty.notify_one();         
                lk_file.unlock();

                create_bulk_file(m); 
            } 

            std::unique_lock<std::mutex> lk_file(task->cv_mx);
            while(!task->empty()) {
                auto m = task->front();

                task->pop();
                create_bulk_file(m); 
            }
        }
        catch(const std::exception &e) {
            eptr = std::current_exception();
            failure = true;           
        }     
    }

    private:      
        std::string main_th_id;
        std::shared_ptr<f_tasks_t> task;      
};


struct PrintData : public Handler
{
    PrintData(std::shared_ptr<p_tasks_t> task_, std::ostream& os_ = std::cout):task(task_), os(os_) {}

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
                if (task->size() <  (MAX_QUEUE_SIZE / 2)) task->cv_empty.notify_one();
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
    std::shared_ptr<p_tasks_t> task;
    std::ostream& os;   
};