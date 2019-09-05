#ifndef COUNTER_H
#define COUNTER_H
#include <string>
#include <queue>
#include <vector>
#include <utility>

namespace counter {

typedef std::pair<std::string, long long> pair;
struct greater {
    bool operator() (const pair& left, const pair& right) {
        return left.second > right.second;
    }
};
typedef std::priority_queue<counter::pair, std::vector<counter::pair>, counter::greater> PairMinHeap;

}

class Counter {
private:
    int id;
    counter::PairMinHeap *min_heap;
public:
    Counter() = delete;
    Counter(const Counter&) = delete;
    Counter(int _id, counter::PairMinHeap *_min_heap) : id(_id), min_heap(_min_heap) { };
    void Run(void);
};


#endif /* COUNTER_H */
