#include "bulk_handlers.h"

//const size_t FILE_THREADS = 4;
std::shared_ptr<Command> cmd;

std::shared_ptr<PrintData> data_log;
std::vector<std::shared_ptr<WriteData>> file_log;


std::thread th_log;
std::vector<std::thread> th_file_log;

static void get_data(unsigned long long N, std::istream& is, std::ostream& os, std::ostream& es, size_t file_th_cnt)
{
    cmd = std::make_shared<Command>(N);

    data_log = std::make_shared<PrintData>(os);
    th_log = std::thread(&PrintData::on_bulk_resolved,   data_log, std::ref(msgs));

    file_log.resize(file_th_cnt);
    th_file_log.resize(file_th_cnt);
    for(size_t i = 0; i < file_th_cnt; ++i){
        file_log[i] = std::make_shared<WriteData>(es);
        th_file_log[i] = std::thread(&WriteData::on_bulk_resolved_file, file_log[i],  std::ref(file_msgs));
    }

    try {     
        for(std::string line; std::getline(is, line);){ 
            if(line.size() > MAX_CMD_LENGTH){
                std::string msg = "Invalid command length. Command length must be < " 
                                  + std::to_string(MAX_CMD_LENGTH) + " :" + line + ".\n";
                throw std::invalid_argument(msg.c_str());
            }
            cmd->on_new_cmd(line);
        }
        cmd->on_cmd_end();
    }
    catch(const std::exception &e) {
        data_log->quit = true;
        cv.notify_one();

        for(size_t i = 0; i < file_th_cnt; ++i){
            file_log[i]->quit  = true;
        }
        cv_file.notify_all();

        th_log.join();
        for(size_t i = 0; i < file_th_cnt; ++i){
            th_file_log[i].join();
        }
        throw;
    }

    data_log->quit = true;
    std::unique_lock<std::mutex> lk(cv_m);  // -fsanitize=thread hung empty test 1:10
    cv.notify_one();
    lk.unlock();

    for(size_t i = 0; i < file_th_cnt; ++i){
        file_log[i]->quit  = true;
    }
    std::unique_lock<std::mutex> lk_file(cv_m_file); // -fsanitize=thread hung empty test 1:10
    cv_file.notify_all();
    lk_file.unlock();

    th_log.join();
    for(size_t i = 0; i < file_th_cnt; ++i){
        th_file_log[i].join();
    }
}

void print_metrics(std::ostream& os)
{
    os << std::endl;

    os << "Thread main   - "; 
    cmd->print_metrics(os);
    os << "Thread log    - " ;
    data_log->print_metrics(os);

    for(size_t i = 0; i < file_log.size(); ++i){
        os << "Thread file_" + std::to_string(i + 1) + " - ";
        file_log[i] ->print_metrics(os);
    }
}


void process(const char* cin_str, std::istream& is = std::cin, 
                                  std::ostream& os = std::cout, 
                                  std::ostream& es = std::cerr,
                                  bool is_metrics = true, 
                                  size_t file_th_cnt = 2)
{
    std::string msg = "Invalid block size. Block size must be > 0  and < " + std::to_string(MAX_BULK_SIZE) + ".\n";
    unsigned long long N;

    std::string str = cin_str;
    if(!std::all_of(str.begin(), str.end(), ::isdigit))
        throw std::invalid_argument("Invalid syntax. Block size must contain only decimal digits.");

    try {
        N = std::stoull (str);
    }
    catch(const std::exception &e) {
        throw std::invalid_argument(msg.c_str());
    }

    if((N == 0) || (N > MAX_BULK_SIZE)){       
        throw std::invalid_argument(msg.c_str());
    }

    get_data(N, is, os, es, file_th_cnt);

    if(is_metrics) print_metrics(os); 
}