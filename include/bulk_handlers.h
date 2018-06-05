#include "command.h"
#include <fstream>


struct WriteData : public Handler
{
    WriteData(int64_t start_time_): start_time(start_time_) {}

    void start(void) override
    {
        helper_thread = std::thread(&WriteData::on_bulk_resolved, 
                                    this,  
                                    std::ref(file_msgs));
    } 


    void create_bulk_file(f_msg_type& msg) 
    {
        p_data_type v;
        std::time_t t;
        size_t id;
        std::tie(v, t, id) = msg;

        std::string file_name = "bulk" + std::to_string(t)  + "_" 
                                       + std::to_string(start_time) + "_"
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


    void on_bulk_resolved(queue_f_msg_type &task) 
    {
        try{
            while(!quit){
                std::unique_lock<std::mutex> lk_file(cv_m_file);

                cv_file.wait(lk_file, [&task, this](){ return (!task.empty() || this->quit); });

                if(task.empty()) break;

                auto m_ex = task.front();

                f_msg_type m;
                //std::shared_ptr<bool> b;
                bool* b;

                std::tie(m, b) = m_ex;
                task.pop();          
                lk_file.unlock();

                create_bulk_file(m);

                std::lock_guard<std::mutex> lk_ext_data(cv_m_data_ext);
                *b = false;

                cv_data_ext.notify_one();  
            } 

            std::lock_guard<std::mutex> lk_file(cv_m_file);
            while(!task.empty()) {
                auto m_ex = task.front();

                f_msg_type m;
                //std::shared_ptr<bool> b;
                bool* b;
                std::tie(m, b) = m_ex;
                task.pop();
                create_bulk_file(m);

                std::lock_guard<std::mutex> lk_ext_data(cv_m_data_ext);
                *b = false;  
                cv_data_ext.notify_one();  
            }
        }
        catch(const std::exception &e) {
            eptr = std::current_exception();
            failure = true;           
        }     
    }

    private:
        int64_t start_time;
};


struct PrintData : public Handler
{
    PrintData(std::ostream& os_ = std::cout):os(os_) {}

    void start(void) override
    {
        helper_thread = std::thread( &PrintData::on_bulk_resolved,
                                     this, 
                                     std::ref(msgs));
    } 


    void on_bulk_resolved(queue_msg_type &task) 
    {
        try {
            while(!quit){
                std::unique_lock<std::mutex> lk(cv_m);

                cv.wait(lk, [&task, this](){ return (!task.empty() || this->quit); });

                if(task.empty()) break;

                auto v = task.front();
                task.pop();
                if (task.size() <  (CIRCLE_BUFF_SIZE / 2)) cv_empty.notify_one();
                lk.unlock();
                
                stream_out(v, os);
                os << "\n";
            } 

            std::lock_guard<std::mutex> lk(cv_m);
            while(!task.empty()) {
                auto v = task.front();
                task.pop();
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
    std::ostream& os;
};