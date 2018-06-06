#define BOOST_TEST_MODULE test_main

#include "test_helper.h"
#include <cstdio>

size_t test_id = 500;

static void gen_static_bulk_test_input_file(std::string file_name, int size)
{
    std::ofstream f{file_name, std::ofstream::trunc};

    BOOST_CHECK_EQUAL( f.good(), true );

    for(auto i = 0; i < size; ++i){
        f << "cmd_" << i << "\n";
    }

    f.close();
}


static void gen_static_bulk_test_output_file(const std::string& file_name, 
                                      int str_size, 
                                      int bulk_size)
{

    std::ofstream f{file_name, std::ofstream::trunc};

    BOOST_CHECK_EQUAL( f.good(), true );
    int i;
    for(i = 0; i < (str_size - bulk_size);){
        f << "bulk: " << "cmd_" << i++;
        for(auto j = 1; j < bulk_size; ++j, ++i){
            f << ", " << "cmd_" << i;
        }
        f << '\n';
    }
    if(i < str_size){
         f << "bulk: " << "cmd_" << i++;
         for(; i < str_size; ++i){
            f << ", " << "cmd_" << i;
         }
         f << '\n';
    }

    f.close(); 
}


static void compare_files(const std::string& file_name_1, const std::string& file_name_2)
{
    std::ifstream ifs1(file_name_1);
    std::ifstream ifs2(file_name_2);

    std::istream_iterator<char> b1(ifs1), e1;
    std::istream_iterator<char> b2(ifs2), e2;

    BOOST_CHECK_EQUAL_COLLECTIONS(b1, e1, b2, e2);
}


static void check_log_files(std::time_t init_time, int blk_num)
{
    std::ifstream  ethalon_file {"ethalon_output.txt"};
    BOOST_CHECK_EQUAL( ethalon_file.good(), true );

    int id = 1;
    std::string ethalon_line;
    while (std::getline(ethalon_file, ethalon_line)){
        std::string name = "bulk" + std::to_string(init_time) + "_" 
                                  + std::to_string(test_id) + "_" 
                                  + std::to_string(id) + ".log";
        
        int watchdog_sec = 0;
        while(!(std::ifstream{name}) && (watchdog_sec < 10)){
            name = "bulk" + std::to_string(++init_time) + "_" 
                          + std::to_string(test_id) + "_" 
                          + std::to_string(id) + ".log";
            ++watchdog_sec;
        }
        if(watchdog_sec >= 10) break;

        std::ifstream log_file {name};
        BOOST_CHECK_EQUAL( log_file.good(), true );

        std::stringstream log_buff;
        log_buff << log_file.rdbuf();

        BOOST_CHECK_EQUAL( log_buff.str(), ethalon_line );
        ++id;
    }
    BOOST_CHECK_EQUAL( --id, blk_num );
}


static void run_process(int str_num, int blk_size, int file_threads_cnt)
{
    std::stringstream oss;
    std::time_t init_time;
    int blk_num = str_num / blk_size + ((str_num % blk_size) == 0 ? 0 : 1);

    gen_static_bulk_test_input_file("test_input.txt", str_num);
    gen_static_bulk_test_output_file("ethalon_output.txt", str_num, blk_size);

    std::ifstream  in_f {"test_input.txt"};
    std::ofstream out_f {"test_out.txt", std::ofstream::trunc};
    BOOST_CHECK_EQUAL( in_f.good(), true );
    BOOST_CHECK_EQUAL( out_f.good(), true );
    
    init_time = std::time(nullptr);
    process(std::to_string(blk_size).c_str(), 
            std::to_string(++test_id), 
            in_f, out_f, false, 
            file_threads_cnt);

    in_f.close();
    out_f.close();

    compare_files("ethalon_output.txt", "test_out.txt"); 
    check_log_files(init_time, blk_num);
}


BOOST_AUTO_TEST_SUITE(test_suite_data_integrity)

BOOST_AUTO_TEST_CASE(data_check)
{
    run_process(10, 3, 1);  

    run_process(10, 3, 2); 

    run_process(100, 3, 4);

    run_process(40000, 100, 2);
}


BOOST_AUTO_TEST_CASE(remove_test_files)
{
    std::system("rm -f *.log");
    std::system("rm -f ethalon_output.txt");
    std::system("rm -f test_input.txt");
    std::system("rm -f test_out.txt");
}
BOOST_AUTO_TEST_SUITE_END()