#include "functions.h"

int main(int argc, char* argv[])
{
    if (argc != 2) {        
        std::cerr << "Usage: " << argv[0] << " N" << std::endl;
        return 1;
    }
    else{

        try {
            process(argv[1]);
        }
        catch(const std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }    
    return 0;
}