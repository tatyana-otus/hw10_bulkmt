#define BOOST_TEST_MODULE test_main

#include "test_helper.h"


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
                                      int bulk_size) {

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

static void run_process(int str_num, int blk_size, int file_threads_cnt)
{
    std::stringstream oss;
    std::stringstream ess;

    gen_static_bulk_test_input_file("test_input.txt", str_num);
    gen_static_bulk_test_output_file("ethalon_output.txt", str_num, blk_size);

    std::ifstream  in_f {"test_input.txt"};
    std::ofstream out_f {"test_out.txt", std::ofstream::trunc};
    BOOST_CHECK_EQUAL( in_f.good(), true );
    BOOST_CHECK_EQUAL( out_f.good(), true );
  
    process(std::to_string(blk_size).c_str(), in_f, out_f, ess, false, file_threads_cnt);

    in_f.close();
    out_f.close();

    compare_files("ethalon_output.txt", "test_out.txt"); 
    check_metrics(str_num, str_num ,
                  str_num / blk_size + ((str_num % blk_size) == 0 ? 0 : 1));

}

BOOST_AUTO_TEST_SUITE(test_suite_data_integrity)


BOOST_AUTO_TEST_CASE(data_check)
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    run_process(10, 3, 1);  

    std::this_thread::sleep_for(std::chrono::seconds(1));
    run_process(10, 3, 2); 

    std::this_thread::sleep_for(std::chrono::seconds(1));
    run_process(100, 3, 4);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    run_process(400000, 100, 2);
}

BOOST_AUTO_TEST_SUITE_END()