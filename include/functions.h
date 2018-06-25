#include "bulk_handlers.h"


void process(const char* cin_str, std::string main_th_id,
                                  std::istream& is = std::cin, 
                                  std::ostream& os = std::cout, 
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

    std::shared_ptr<PrintData> data_log;
    std::vector<std::shared_ptr<WriteData>> file_log;

    auto  q_file  = std::make_shared<f_tasks_t>();
    auto  q_print = std::make_shared<p_tasks_t>();

    auto cmd = std::make_unique<Command>(N, q_file, q_print);

    data_log = std::make_shared<PrintData>(q_print, os); 
    file_log.resize(file_th_cnt);
    for(size_t i = 0; i < file_th_cnt; ++i){
        file_log[i] = std::make_shared<WriteData>(q_file, main_th_id);
    }

    cmd->add_hanlder(data_log);
    for(auto& h : file_log){
        cmd->add_hanlder(h);
    }

    cmd->get_data(is);

    if(is_metrics) {
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
}