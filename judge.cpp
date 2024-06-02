#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <algorithm>
#include <vector>

// there are leading and trailing double quotation marks for src_path so g++ can still compile
// if there are spaces within the path name
std::string src_path = "\"path/to/src_code.cpp\"";
std::string test_data_path = "path/to/test_data";
double time_limit = 2.0; // 2 s = 2000 ms
double memory_limit = 256; // 256mb
std::string compiler_flags = " -std=c++17 -O2 -lm -Wall "; // keep the leading and trailing spaces

void check(int exit_code, int test_number) {
    /*
     * return codes:
     * AC: accepted solution
     * TLE: time limit exceeded
     * MLE: memory limit exceeded
     * RE: runtime error
     * WA: wrong answer
     */

    // check for exit code
    if (exit_code != 0) {
        std::cout << "\x1b[31m" << "RE with exit code: " << exit_code << "\x1b[0m";
        return;
    }

    std::ifstream user_out("user_out.txt");
    std::ifstream expected(test_data_path + "/" + std::to_string(test_number) + ".out");

    std::vector<std::string> output;
    std::string line;
    while (std::getline(user_out, line)) {
        // no way any output for a program will have "user" AND "system" AND "elapsed" right??? right?!?
        // there is no way
        // no possible way
        bool probably_time = line.find("user") != std::string::npos &&
                             line.find("system") != std::string::npos &&
                             line.find("elapsed") != std::string::npos;

        if (!probably_time) {
            output.push_back(line);
        } else {
            break;
        }
    }

    // now line contains information about time and memory usage:

    // check for time limit
    std::string time = line.substr(line.find(':') + 1);
    double execution_time = std::stod(time);
    if (execution_time > time_limit) {
        std::cout << "\x1b[31m" << "TLE\n" << "\x1b[0m";
        return;
    }
    // check for memory limit
    std::string mem = line.substr(line.find("avgdata ") + 8);
    double memory_used = std::stod(mem) / 1000;
    if (memory_used > memory_limit) {
        std::cout << "\x1b[31m" << "MLE\n" << "\x1b[0m";
        return;
    }

    // check for wrong answer
    std::vector<std::string> expected_out;
    while (std::getline(expected, line)) {
        expected_out.push_back(line);
    }

    if (expected_out != output) {
        std::cout << "\x1b[31m" << "WA" << "\x1b[0m";
    } else {
        std::cout << "\x1b[92m" << "AC" << "\x1b[0m";
    }

    // output execution time and memory usage
    std::cout << "   \t" << execution_time * 1000 << " ms " << memory_used << " mb";
}

void run(int test_number) {
    // base cmd: "./{executable_name}", simply runs the program
    std::string cmd = "TIMEFORMAT='%4R'; gtime ./submission";// + src_path;

    // redirect input and output:
    std::string input_name = test_data_path + "/" + std::to_string(test_number) + ".in";
    cmd += " < " + input_name;
    cmd += " > user_out.txt"; // program output, execution time, memory usage written here

    // Open the output file for writing
    std::ofstream user_out("user_out.txt");
    if (!user_out.is_open()) {
        std::cerr << "Error opening file: user_out.txt" << std::endl;
        return;
    }

    // "2>&1" directs stderr to the same output location as stdout, either a file or the console
    cmd += " 2>&1";

    // Open a pipe for reading the time command's output
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error opening pipe" << std::endl;
        user_out.close();
        return;
    }

    // Read from the pipe (stdin) char by char and write to the file
    char ch;
    while ((ch = static_cast<char>(fgetc(pipe))) != EOF) {
        std::cout << ch;
        user_out << ch;

        if (ferror(pipe)) {
            std::cerr << "Error reading from pipe" << std::endl;
            pclose(pipe);
            return;
        }
    }

    int exit_code = pclose(pipe); // close the pipe and get the return code from the child
    user_out.close();  // Close the output file

    std::cout << "Verdict for test case " << test_number << ": ";
    check(exit_code, test_number);
    std::cout << std::endl;
}

int compile() {
    std::string compile_command = "g++" + compiler_flags + src_path + " -o submission";
    FILE* pipe = popen(compile_command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error: Could not open pipe for compilation" << std::endl;
        return 1;
    }

    // Read compiler output line by line and display it
    char ch;
    while ((ch = static_cast<char>(fgetc(pipe))) != EOF) {
        std::cout << ch;

        if (ferror(pipe)) {
            std::cerr << "Error reading from pipe" << std::endl;
            pclose(pipe);
            return 1;
        }
    }

    int exit_code = pclose(pipe);

    // Check compilation exit code
    if (exit_code != 0) {
        std::cerr << "Compilation failed\n" << std::endl;
        return 1;
    } else {
        std::cout << "Compilation successful\n" << std::endl;
        return 0;
    }
}

int main() {
    int ret_code = compile();
    if (ret_code != 0) {
        return 1;
    }

    // iterator to the directory that the test data is stored in
    auto directory_it = std::filesystem::directory_iterator(test_data_path);

    // division by 2 since there are two files per test case n: n.in, n.out
    int num_test_cases = std::count_if(
            begin(directory_it),
            end(directory_it),
            [](auto& entry) { return entry.is_regular_file(); }
    ) / 2;

    std::cout << "Results are displayed below:" << std::endl;
    for (int i = 1; i <= num_test_cases; i++) {
        // run executable with test case i as input
        run(i);
    }
    std::filesystem::remove("user_out.txt");
    std::filesystem::remove("submission");
}
