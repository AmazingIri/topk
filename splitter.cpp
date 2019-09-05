#include <sys/stat.h>
#include <string>
#include <fstream>
#include <vector>
#include <cstdio>

extern long long g_num_files;

void SplitInputFile(std::string input_file_name) {
    std::ifstream input_file;
    input_file.open(input_file_name, std::ios::binary);

    std::vector<std::ofstream> inter(g_num_files);
    mkdir("tmp", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    for (int i = 0; i < g_num_files; ++i) {
        std::string path = "tmp/temp_" + std::to_string(i) + ".txt";
        inter[i].open(path, std::ios::binary);
    }

    std::string str;
    long long counter = 0;
    while (input_file >> str) {
        auto idx = std::hash<std::string>{}(str) % g_num_files;
        inter[idx] << str << std::endl;
        if (++counter % 1000000 == 0) {
            printf("Splited %lld million rows of input file...\n", counter / 1000000);
            fflush(stdout);
        }
    }

    for (int i = 0; i < g_num_files; ++i) {
        inter[i].close();
    }

    input_file.close();
}

int RemoveTmp(void) {
    return system("rm -rf ./tmp");
}
