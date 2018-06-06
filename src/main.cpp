#include "functions.h"

int main(int argc, char* argv[])
{
    if (argc != 2) {        
        std::cerr << "Usage: " << argv[0] << " N" << std::endl;
        return 1;
    }
    else{

        try {
            std::stringstream ss;
            ss << std::this_thread::get_id();
            auto main_th_id = ss.str();

            process(argv[1], main_th_id);
        }
        catch(const std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }    
    return 0;
}