#include <unistd.h>
#include <cstdio>
#include <fstream>
#include <sys/sysinfo.h>
#include <string>
#include <mutex>
#include <vector>
#include <thread>
#include "splitter.h"
#include "counter.h"

int g_num_cores = -1;
long long g_num_files = -1; // number of intermediate files
long long g_file_queue = -1; // idx of the current remaining file
std::mutex g_queue_lock;
static const long long available_memory = 1ll * 1024 * 1024 * 1024;

// see README 3rd section
// we estimate the hash table will take 15 times of the original file, so 15+1=16
static const int mem_enlarge = 16; 

void Calculate_g_num_files(std::string input_file_name);

int main(int argc, char *argv[]) {
    int opt;
    std::string input_file_name = "input.txt";
    std::string output_file_name = "output.txt";
    bool skip_split = false;
    bool keep_tmp = false;

    while ((opt = getopt(argc, argv, "i:o:sk")) != -1) {
        switch (opt) {
        case 'i':
            input_file_name = std::string(optarg);
            break;
        case 'o':
            output_file_name = std::string(optarg);
            break;
        case 's':
            skip_split = true;
            keep_tmp = true;
            break;
        case 'k':
            keep_tmp = true;
            break;
        default:
            printf("Usage: %s -i input_file_name -o output_file_name -s -k\n", argv[0]);
            printf("\t-s: skip spliting file, reuse /tmp, will include -k\n");
            printf("\t-k: keep /tmp, do not delete /tmp at the end\n");
            return -1;
            break;
        }
    }

    Calculate_g_num_files(input_file_name);
    
    // 1. split the input file into intermediate file. Single core, takes most of the time
    auto start = std::chrono::system_clock::now();
    if (!skip_split) {
        SplitInputFile(input_file_name);
    }
    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    if (!skip_split) {
        printf("File Splitted, took %ld seconds\n", elapsed);
    }

    // 2. use counters to count intermediate files. multi-threaded
    // 2.1 create Counters and min_heaps
    start = std::chrono::system_clock::now();
    g_file_queue = g_num_files - 1;
    std::vector<Counter*> counters(g_num_cores);
    std::vector<counter::PairMinHeap> min_heaps(g_num_cores);
    for (int i = 0; i < g_num_cores; ++i) {
        counters[i] = new Counter(i, &min_heaps[i]);
    }
    
    // 2.2 start workers
    std::vector<std::thread> threads(g_num_cores);
    for (int i = 0; i < g_num_cores; ++i) {
        threads[i] = std::thread(&Counter::Run, counters[i]);
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_setaffinity_np(threads[i].native_handle(), sizeof(cpu_set_t), &cpuset);
    }
    for (auto &t : threads) {
        t.join();
    }
    end = std::chrono::system_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    printf("All intermediate files have been counted, took %ld seconds\n", elapsed);
    for (int i = 0; i < g_num_cores; ++i) {
        delete counters[i];
    }

    // 3. k-way merge all the heaps. k = number of cores (instead of number of files)
    // 3.1 move values from min_heap to vector
    start = std::chrono::system_clock::now();
    std::vector<std::vector<counter::pair>> results_per_core;
    for (auto it = min_heaps.begin(); it != min_heaps.end(); ++it) {
        int size = it->size(); // normally should be 100
        std::vector<counter::pair> result(size); 
        // result is in ascending order, so later on when doing multi-way merge, just remove from the back
        for (int i = 0; i < size; ++i) {
            result[i] = it->top();
            it->pop();
        }
        results_per_core.push_back(result);
    }

    // 3.2 k-way merge sort
    std::vector<counter::pair> results;
    while (results.size() < 100) {
        int idx = -1;
        long long largest = 0;
        for (auto it = results_per_core.begin(); it != results_per_core.end(); ++it) {
            if (!it->empty()) {
                counter::pair cur = it->back(); // ascending order, the back is the largest
                if (cur.second > largest) {
                    largest = cur.second;
                    idx = it - results_per_core.begin();
                }
            }
        }
        if (idx == -1) // total unique URL count < 100
            break;
        results.push_back(results_per_core[idx].back());
        results_per_core[idx].pop_back(); // pop_back, don't have to move the entire vector
    }
    end = std::chrono::system_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    printf("%d-way merge finished, took %ld MICROseconds\n", g_num_cores, elapsed);

    std::ofstream output_file;
    output_file.open(output_file_name, std::ios::binary);
    for (auto it : results) {
        output_file << it.first.c_str() << " " << it.second << std::endl;
        printf("%s %lld\n", it.first.c_str(), it.second);
    }
    output_file.close();
    printf("Results have been output to %s.\n", output_file_name.c_str());    

    if (!keep_tmp)
        RemoveTmp();
    return 0;
}

void Calculate_g_num_files(std::string input_file_name) {
    std::ifstream input_file;
    input_file.open(input_file_name, std::ios::binary | std::ios::ate); // exception aborts the program, which is what we wanted

    long long input_file_size = input_file.tellg();
    if (input_file_size == -1) {
        input_file.close();
        throw std::invalid_argument("Input file does not exist");
    }
    g_num_cores = get_nprocs();
    if (g_num_cores == 0) {
        input_file.close();
        throw std::runtime_error("core number from get_nprocs() error");
    }
    auto inter_file_size = available_memory / (g_num_cores * mem_enlarge);
    g_num_files = input_file_size / inter_file_size;
    if (g_num_files == 0)
        g_num_files = 1; // even smaller than inter_file_size
    printf("System has %d processors available; input filesize %.1lf GB.\n"
        "We will have %lld intermediate files, each file size %.2lf MB.\n",
        get_nprocs(), (double)input_file_size / (1024*1024*1024),
        g_num_files, (double)inter_file_size / (1024*1024));
    input_file.close();
}
