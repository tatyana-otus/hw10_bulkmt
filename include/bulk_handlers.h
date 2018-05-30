#include "command.h"
#include <fstream>



struct WriteData : public Handler
{
    WriteData(std::ostream& es_ = std::cout):es(es_) {}

    void create_bulk_file(f_msg_type& msg) 
    {
        p_data_type v;
        std::time_t t;
        size_t id;
        std::tie(v, t, id) = msg;

        std::string file_name = "bulk" + std::to_string(t) + "_" + std::to_string(id) + ".log";
        
        if(!std::ifstream{file_name}){

            std::ofstream of{file_name};

            stream_out(v, of);

            of.close();
        }
        else {
            std::unique_lock<std::mutex> lk(std_err_mx);
            es << file_name << " log file already exists" << std::endl;
        }    
    }


    void on_bulk_resolved_file(queue_f_msg_type &task) 
    {

        while(!quit){
            std::unique_lock<std::mutex> lk_file(cv_m_file);

            cv_file.wait(lk_file, [&task, this](){ return (!task.empty() || this->quit); });

            if(task.empty()) break;


            auto m_ex = task.front();

            f_msg_type m;
            std::shared_ptr<bool> b;
            std::tie(m, b) = m_ex;
            task.pop();          
            lk_file.unlock();
            create_bulk_file(m);

            std::unique_lock<std::mutex> lk_ext_data(cv_m_data_ext);
            *b = false;  
            cv_f_empty.notify_one();  
            lk_ext_data.unlock();
        } 

        std::unique_lock<std::mutex> lk_file(cv_m_file);
        while(!task.empty()) {
            auto m_ex = task.front();

            f_msg_type m;
            std::shared_ptr<bool> b;
            std::tie(m, b) = m_ex;
            task.pop();
            create_bulk_file(m);

            std::unique_lock<std::mutex> lk_ext_data(cv_m_data_ext);
            *b = false;  
            cv_f_empty.notify_one();  
            lk_ext_data.unlock();
        }
        lk_file.unlock(); 
    }

    private:
        std::ostream& es; 
};


struct PrintData : public Handler
{
    PrintData(std::ostream& os_ = std::cout):os(os_) {}


    void on_bulk_resolved(queue_msg_type &task) 
    {
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

        std::unique_lock<std::mutex> lk(cv_m);
        while(!task.empty()) {
            auto v = task.front();
            task.pop();
            stream_out(v, os);
            os << '\n';
        }
        lk.unlock(); 
    }

private:
    std::ostream& os;
};