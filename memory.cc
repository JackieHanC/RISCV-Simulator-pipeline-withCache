#include "memory.h"
#include <assert.h>

void Memory::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int &hit, int &time) {
	printf("bytes is %d\n",bytes );
	assert( (bytes == 1) || (bytes == 4) ||
			(bytes == 8) || (bytes == 16) || (bytes == 64));
	if (read == 1) {
		printf("read\n");
		int i = 0;
		char * begin_addr = this->memory + addr;
		while(i != bytes) {
			*(content + i) = *(begin_addr + i);

			i++;
		}
		printf("Here\n");
	} else if (read == 0) {
		printf("write\n");
		int i = 0;
		char * begin_addr = this->memory + addr;
		printf("Here\n");
		while(i != bytes) {

			*(begin_addr + i) = *(content + i);
			i++;
		}
		printf("Here\n");
	} else {
		printf("read num error\n");
		assert(false);
	}
    hit = 1;
    time = latency_.hit_latency + latency_.bus_latency;
    stats_.access_time += time;
}

