#include "stdio.h"
#include "cache.h"
#include "memory.h"
#include <fstream>
using namespace std;

int main(void) {
    Memory m;
    Cache l1;
    Cache l2;
    Cache llc;
    l1.SetLower(&l2);
    l2.SetLower(&llc);
    llc.SetLower(&m);


    StorageStats s;
    s.access_time = 0; 
    m.SetStats(s);
    l1.SetStats(s);

    StorageLatency ml;
    ml.bus_latency = 6;
    ml.hit_latency = 100;
    m.SetLatency(ml);

    StorageLatency ll;
    ll.bus_latency = 3;
    ll.hit_latency = 10;
    l1.SetLatency(ll);


    // set the cache config
    CacheConfig_ config;

    config.size = 32 MB;

    config.associativity = 256;

    config.set_num = 128;


    config.write_through = 0;
    config.write_allocate = 1;

    l1.SetConfig(config);



    int hit, time;
    char content[64];
// #ifdef DEBUG
//     printf("HandleRequest 1\n");
// #endif 
//     l1.HandleRequest(0, 0, 1, content, hit, time);
//     printf("Request access time: %dns\n", time);

// #ifdef DEBUG
//     printf("HandleRequest 2\n");
// #endif
//     l1.HandleRequest(1024, 0, 1, content, hit, time);
//     printf("Request access time: %dns\n", time);
    ifstream trace_file("../trace/2.trace");

    char c;
    uint64_t addr;
    int read;
    while(trace_file >> c >> addr) {
        read = (c == 'r' ? 1:0);
        l1.HandleRequest(addr, 0, read, content, hit, time);

        printf("\n%c  %llu\n", c, addr);
        printf("Request access time: %dns\n", time);
    }


    l1.GetStats(s);
    printf("Total L1 access time: %dns\n", s.access_time);
    printf("L1 access count is %d\n",s.access_counter);
    printf("L1 miss num is %d\n",s.miss_num );
    printf("L1 miss rate %.2f%%\n", s.miss_num/(float)s.access_counter * 100);
    m.GetStats(s);
    printf("Total Memory access time: %dns\n", s.access_time);
    return 0;
}
