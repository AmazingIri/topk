#include <mutex>
#include <unistd.h>
#include <string>
#include <fstream>
#include <unordered_map>
#include "counter.h"

#include <cstdio>


extern long long g_file_queue;
extern std::mutex g_queue_lock;

void Counter::Run(void) {
    while (1) {
        long long cur_file_idx;
        g_queue_lock.lock();
        if (g_file_queue == -1) {
            g_queue_lock.unlock();
            break;
        }
        cur_file_idx = g_file_queue--;
        if (cur_file_idx % 20 == 19)
            printf("%lld files left to be counted...\n", cur_file_idx + 1);
        g_queue_lock.unlock();

        std::unordered_map<std::string, long long> hashmap;
        std::string filepath = "tmp/temp_" + std::to_string(cur_file_idx) + ".txt";
        std::ifstream file;
        file.open(filepath, std::ios::binary);

        std::string str;
        while (file >> str) {
            ++hashmap[str];
        }

        for (auto pair : hashmap) {
            if (min_heap->size() < 100) {
                min_heap->push(pair);
            } else {
                auto top = min_heap->top();
                // if this URL's click is bigger than the smallest of 100
                if (pair.second > top.second) {
                    min_heap->pop();
                    min_heap->push(pair);
                }
            }
        }
        //printf("thread %d on core %d counted tmp_%lld.txt \n", id, sched_getcpu(), cur_file_idx);
        file.close();
    }
}