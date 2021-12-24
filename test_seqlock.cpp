#include "hi_seqlock.h"
#include <string.h>
#include <stdio.h>
#include <thread>

#define NUM 15
#define max_read_timeout_hz (2500*90) // 90us

using namespace std;

struct share_data {
    uint64_t data[NUM];
    uint64_t version;
};

// only use seqlock[0]
hi_seqlock_t seqlock[256];

share_data race;

static void write_data() {
    for (uint32_t i = 15; i < 30; i++) {
        race.data[i % NUM] = race.data[(i - 1) % NUM] + race.data[(i - 2) % NUM];
        if (i == 25) usleep(10);
    }
    race.version++;
}

static void verify_data(share_data& my, uint64_t& prev_ver) {
    if (prev_ver > my.version) {
        printf("version error: %lu != %lu\n", prev_ver, my.version);
        exit(-1);
    }
    for (int i = 2; i < NUM; i++) {
        if (my.data[i] != my.data[i-1] + my.data[i-2]) {
            printf("data error: data[%d] != data[%d] + data[%d], %lu != %lu + %lu\n",
                    i, i-1, i-2, my.data[i], my.data[i-1], my.data[i-2]);
            exit(-1);
        }
    }
    prev_ver = my.version;
}

void reader_func() {
    uint64_t timeout_cnt = 0, success_cnt = 0, retry_cnt = 0, retry_total = 0;
    uint64_t prev_ver = 0, prev_loop1 = 0, prev_loop2 = 0;
    share_data my;

    for (uint32_t i = 0; ; i++) {
        uint64_t retry = 0;
        uint32_t seq;
        do {
            seq = read_seqbegin(&seqlock[0], max_read_timeout_hz);
            if (is_read_seqbegin_timeout(seq)) {
                break;
            }

            // read race resource
            memcpy(&my, &race, sizeof(race));

            retry++;
        } while (read_seqretry(&seqlock[0], seq));

        // handle timeout
        if (is_read_seqbegin_timeout(seq)) {
            timeout_cnt++;
            if (i - prev_loop1 >= 10000*10) {
                printf("read_seqbegin timeout, seq = %u, tiemout_num = %lu, success_cnt = %lu\n",
                    seq, timeout_cnt, success_cnt);
                prev_loop1 = i;
            }
            continue;
        }

        verify_data(my, prev_ver);
        success_cnt++;

        if (retry > 1) {
            retry_total += retry;
            retry_cnt++;
            if (i - prev_loop2 >= 100000) {
                printf("read_seqretry total %lu, cnt %lu, avg %lf\n",
                    retry_total, retry_cnt, (double)retry_total / (double)retry_cnt);
                prev_loop2 = i;
            }
        }
        usleep(10);
    }
}

void writer_func() {
    while (1) {
        write_seqlock(&seqlock[0]);

        write_data();

        write_sequnlock(&seqlock[0]);

        usleep(234);
    }
}

// g++ -std=c++11 -O2 test_seqlock.cpp -I. -pthread
int main() {
    race.data[0] = race.data[1] = 1;

    int reader_num = 7;
    thread reader[reader_num];
    thread writer;

    writer = thread(writer_func);
    for (int i=0; i<reader_num; i++) {
        reader[i] = thread(reader_func);
    }

    writer.join();
    for (int i=0; i<reader_num; i++) {
        reader[i].join();
    }
    return 0;
}
