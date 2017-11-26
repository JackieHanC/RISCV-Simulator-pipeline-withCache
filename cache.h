#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include "storage.h"
#include <math.h>
#include "def.h"
#include <assert.h>

#define KB * (int)(pow(2, 10))

#define MB * (int)(pow(2, 20))


typedef struct CacheConfig_ {
    int size;
    int associativity;
    int set_num; // Number of cache sets
    int write_through; // 0|1 for back|through
    int write_allocate; // 0|1 for no-alc|alc
} CacheConfig;

class line {
public:
    line(int b) {
        valid = false;
        dirty = false;
        tag = 0;
        content = new char[(int)pow(2, b)];
        nextLine = NULL;
        preLine = NULL;
    }
    line *nextLine;
    line *preLine;
    uint64_t tag;
    bool valid;
    char * content;
    bool dirty;
    
};
class set {
public:
    void init(int associativity, int b);

    void setHead(line * visitedLine); 

    line * getEndLine();

    int line_num;
    line * head;

}; 
class Cache: public Storage {
public:
    Cache() {}
    ~Cache() {}

    // Sets & Gets
    void SetConfig(CacheConfig cc) {
        this->config_ = cc;
        init();
    }
    void GetConfig(CacheConfig &cc) {
        cc = this->config_;
    }
    void SetLower(Storage *ll) { lower_ = ll; }
    // Main access process
    void HandleRequest(uint64_t addr, int bytes, int read,
                        char *content, int &hit, int &time);
    void flush(Storage *s);

private:
    void init(); 
    // Bypassing
    int BypassDecision();
    // Partitioning
    void PartitionAlgorithm();
    // Replacement
    int ReplaceDecision(uint64_t addr,int read, int bytes, char * content, 
                int &time);
    line* ReplaceAlgorithm(int &time);

    bool AfterFetachRequest(line* visitedLine, int read, int bytes,
                     char *content);

    uint64_t getRES(uint64_t addr, int rshift, int t);

    // Prefetching
    int PrefetchDecision();
    void PrefetchAlgorithm();

    CacheConfig config_;
    Storage *lower_;
    set * sets;
    int t;
    int s;
    int b;

    // int S;
    // int E;
    // int B;
    uint64_t request_tag;
    uint64_t request_s;
    uint64_t request_offset;
    DISALLOW_COPY_AND_ASSIGN(Cache);
};

#endif //CACHE_CACHE_H_ 
