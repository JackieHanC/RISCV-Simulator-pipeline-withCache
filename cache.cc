#include "cache.h"
void set::init(int associativity, int b) {
    line_num = associativity;
    head = new line(b);
    line * tmp = head;
    for (int i = 0;i < associativity - 1; ++i) {
        tmp->nextLine = new line(b);
        tmp->nextLine->preLine = tmp; 
        tmp = tmp->nextLine;
    }
}

void set::setHead(line *visitedLine) {
    printf("Set Head\n");
    bool inList = false;
    line *tmp = this->head;



    while(tmp != NULL) {
        // the visited line is in this set
        if (visitedLine == tmp) {
            inList = true;
        }
        // if (tmp->valid) {
        //     printf("tag %x(pre tag %x)  ->  ",tmp->tag ,(tmp->preLine == NULL)?
        //         0:tmp->preLine->tag
        //         );
        // }else {
        //     printf("invalid ->");
        // }
        tmp = tmp->nextLine;

    }
    assert(inList);

    if (visitedLine == this->head)
        return;


    line * pre = visitedLine->preLine;
    line * next = visitedLine->nextLine;
    if (pre != NULL)
        pre->nextLine = next;
    if (next != NULL)
        next->preLine = pre;

    visitedLine->preLine = NULL;
    visitedLine->nextLine = this->head;
    this->head->preLine = visitedLine;


    this->head = visitedLine;

    assert(this->head->nextLine->nextLine != this->head);
}

line* set::getEndLine() {
    line * tmp = this->head;
    while(tmp->nextLine != NULL) {
        tmp = tmp->nextLine;
    }
    return tmp;
}


void Cache::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int &hit, int &time) {
    hit = 0;
    time = 0;
    line *visitedLine;
    // Bypass?
    if (!BypassDecision()) {
        PartitionAlgorithm();

        stats_.access_counter++;
        // Miss?

        if (ReplaceDecision(addr, read, bytes, content, time)) {
#ifdef DEBUG
            printf("Request Miss\n");
#endif 
            // Choose victim
            visitedLine = ReplaceAlgorithm(time);
        } else {
#ifdef DEBUG
            printf("Request Hit\n");
#endif 
            // return hit & time
            hit = 1;
            time += latency_.bus_latency + latency_.hit_latency;
            stats_.access_time += time;
            return;
        }
    }

    // miss

    stats_.miss_num++;
    // Prefetch?
    if (PrefetchDecision()) {
        PrefetchAlgorithm();
    } else {
        // Fetch from lower layer
        int lower_hit, lower_time;

        //read miss
        if (read == 1) {
            // the comment below is read through
            // *
            // lower_->HandleRequest(addr, bytes, read, content,
            //                         lower_hit, lower_time);
            // *
            // now we need read allocate
            //  read 2^b bytes from the addr without offset 
            //  to the cache
            uint64_t no_off_addr = (addr >> this->b) << this->b;
            lower_->HandleRequest(no_off_addr, (int)pow(2, this->b),
                read, visitedLine->content, lower_hit, lower_time);
            hit = 0;
            time += latency_.bus_latency + lower_time;
            stats_.access_time += latency_.bus_latency;

            printf("read fetch back\n");
            // now visitedLine is the last line of the set
            visitedLine->valid = TRUE;

            // now seem like we get the content from memory
            // and replace the line of cache 
            visitedLine->tag = this->request_tag;

            // we may not have to update the line ,becase our next
            // fetch we set it as head
            // this->sets[this->request_s].setHead(visitedLine);

            //fetch again
            bool t = AfterFetachRequest(visitedLine, read, bytes, content);
            // it must be hit
            assert(t);
        }
        else if (read == 0) {

            if (this->config_.write_allocate == 1) {
                // get the memory to cache 
                uint64_t no_off_addr = (addr >> this->b) << this->b;
                lower_->HandleRequest(no_off_addr, (int)pow(2, this->b),
                    read, visitedLine->content, lower_hit, lower_time);
                hit = 0;
                time += latency_.bus_latency + lower_time;
                stats_.access_time += latency_.bus_latency;

                printf("write fetch back\n");

                visitedLine->valid = TRUE;
                visitedLine->tag = this->request_tag;
                // then write again
                bool t = AfterFetachRequest(visitedLine, read, bytes, content);
                assert(t);
                if (this->config_.write_through == 0) {
                    visitedLine->dirty = 1;
                }
            }
            else if (this->config_.write_allocate == 0) {
                // write memory directly 
                lower_->HandleRequest(addr, bytes, read, content, 
                                lower_hit, lower_time);
                hit = 0;
                time += latency_.bus_latency + lower_time;
                stats_.access_time += latency_.bus_latency;

            } else {
                printf("write allocate num error\n");
                assert(false);
            }

        } else {
            printf("read num error\n");
            assert(false);
        }
    }

}

void Cache::init() {


    int size = this->config_.size;

    int associativity = this->config_.associativity;
    int set_num = this->config_.set_num;
 
    this->sets = new set[set_num];
    int B = size / (associativity*set_num);


    this->s = (int)(log(set_num)/log(2));
    this->b =  (int)(log(B)/log(2));
    this->t = 64 - this->s - this->b;



#ifdef DEBUG
    printf("size is %d bytes\n", size);  
    printf("associativity is %d\n",associativity );
    printf("set_num is %d\n",set_num);
    printf("s is %d\n", this->s);
    printf("b is %d\n",this->b);
    printf("t is %d\n",this->t);
#endif

    for (int i = 0;i < set_num;++i) {
        this->sets[i].init(associativity, this->b);
    }

}

int Cache::BypassDecision() {
    return FALSE;
}

void Cache::PartitionAlgorithm() {



}

bool
Cache::AfterFetachRequest(line* visitedLine, int read, 
            int bytes, char *content) {
    set * the_set = &(this->sets[this->request_s]);
    if (read == 1) {
        char * content_Baddr = (visitedLine->content + 
                    this->request_offset);

        int i = 0;
        while(i != bytes) {
            *(content + i) = *(content_Baddr + i);
            i++;
        }
        the_set->setHead(visitedLine);
    } else if (read == 0) {
        char * content_Baddr = (visitedLine->content + 
                    this->request_offset);

        int i = 0;
        while(i != bytes) {
            *(content_Baddr + i) = *(content + i);
            i++;
        }
        the_set->setHead(visitedLine);
    }
    else {
        printf("read num error\n");
        assert(false);
    }
    return TRUE;
}

int Cache::ReplaceDecision(uint64_t addr, int read, int bytes, char *content,
                    int &time) {

    this->request_tag = getRES(addr, this->s+this->b,this->t);

    this->request_s = getRES(addr, this->b, this->s);

    this->request_offset = getRES(addr, 0, this->b);
    printf("t is %d, s is %d, b is %d\n",this->t, this->s, this->b );
    printf("Request addr is %x\n", addr);

    printf("This request tag is 0x%x, set num is 0x%x, request offset is 0x%x\n"
        , this->request_tag, this->request_s, this->request_offset);

    set * the_set = &(this->sets[this->request_s]);

    line * tmp = the_set->head;


    while(tmp != NULL) {
        // hit 
// #ifdef DEBUG
//         printf("request_tag is %llu\n",  this->request_tag);
//         printf("tmp tag is %llu\n", tmp->tag);
// #endif

        if (tmp->valid && (this->request_tag == tmp->tag)) {
#ifdef DEBUG
            printf("Tag equals\n");
#endif

            // 1 for read
            if (read == 1) {
#ifdef DEBUG
                printf("read\n");
                printf("offset is %llu\n", this->request_offset);
#endif
                char * content_Baddr = (tmp->content + this->request_offset);

                int i = 0;
                printf("bytes is %d\n",bytes);
                while(i != bytes) {
                    *(content + i) = *(content_Baddr + i);
                    i++;
                }

            }
            // 0 for write 
            else if (read == 0) {
#ifdef DEBUG
                printf("write\n");
#endif
                char *content_Baddr = (tmp->content + this->request_offset);

                int i = 0;
                while(i != bytes) {
                    *(content_Baddr + i) = *(content + i);
                    i++;
                }
                // if the write back policy, we have the dirty bit
                if (config_.write_through == 0) {
                    tmp->dirty = TRUE;
                } 
                // else write through, we need to write the content
                //  into memory and the cache 
                else {
                    int lower_hit, lower_time;
                    lower_->HandleRequest(addr, bytes, read, content,
                                            lower_hit, lower_time);
                    
                    time += latency_.bus_latency + lower_time;
                    stats_.access_time += latency_.bus_latency;
                }

            } else {
                printf("read num error\n");
                return FALSE;
            }


            //hit , move the hit line to head
            the_set->setHead(tmp);
            // line * pre = tmp->preLine;
            // line * next = tmp->nextLine;
            // if (pre != NULL)
            //     pre->nextLine = next;
            // if (next != NULL)
            //     next->preLine = pre;

            // tmp->preLine = NULL;
            // tmp->nextLine = the_set->head;


            // the_set->head = tmp;

            return FALSE;
        }

        tmp = tmp->nextLine;
    }
    // miss , return TRUE
    return TRUE;




}
// Cache::ReplaceAlgorithm
//  Use LRU algorithm. 
//  Cache sets[i] is a double linked list. 
//  The head line is used most frequently.  
//  
line* Cache::ReplaceAlgorithm(int &time){
    // Just simulate replace action

    set * the_set = &(this->sets[this->request_s]);

    line * tmp = the_set->getEndLine();

    // while(tmp->nextLine != NULL) {
    //     tmp = tmp->nextLine;
    // }

    

    // If the policy is write back 
    //  then when replace the line, write it back to lower_ 
    if (this->config_.write_through == 0) {
        // if the line is dirty , then 
        if (tmp->dirty) {

            uint64_t replace_addr = 0 | (this->request_s << this->b) |
                (tmp->tag << (this->b + this->s));
            int lower_hit, lower_time;
            // bytes is the block size , that is 2^b
            lower_->HandleRequest(replace_addr, (int)pow(2,b) , 0, 
                tmp->content, lower_hit, lower_time);

            time += latency_.bus_latency + lower_time;
            stats_.access_time += latency_.bus_latency;
        }
    }

    return tmp;



}

int Cache::PrefetchDecision() {
    return FALSE;
}

void Cache::PrefetchAlgorithm() {
}

uint64_t Cache::getRES(uint64_t addr, int rshift, int t){
    uint64_t res = addr >> rshift;
    for (int i = 0;i < 64;i++) {
        if (i >= t) {
            res &= ~((uint64_t)1 << i);
        }
    }
    return res;
}

void Cache::flush(Storage * s) {
    for (int i = 0;i < this->config_.set_num; i++) {
        line *tmp = this->sets[i].head;
        while(tmp != NULL) {
            
            if (tmp->valid && tmp->dirty) {
                uint64_t addr = 0 | (tmp->tag << this->s+this->b) |
                        (uint64_t(i) << this->b);
                int hit;
                int time;
                s->HandleRequest(addr, (int)pow(2,this->b), 0, tmp->content,
                        hit, time);

            }
            tmp = tmp->nextLine;
        }
    }
}