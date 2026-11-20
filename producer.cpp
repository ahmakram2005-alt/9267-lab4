#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <random>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
using namespace std;

struct Slot {
    char   name[11];
    double price;
};

struct Header {
    int head;
    int tail;
};

// returns the current time as a string
string stamp() {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    tm t;
    localtime_r(&ts.tv_sec, &t);
    char buf[40];
    strftime(buf, sizeof(buf), "%m/%d/%Y %H:%M:%S", &t);
    char out[60];
    snprintf(out, sizeof(out), "%s.%03ld", buf, ts.tv_nsec / 1000000);
    return string(out);
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        fprintf(stderr, "usage: %s NAME MEAN SD SLEEP_MS N\n", argv[0]);
        return 1;
    }
    string cname = argv[1];
    double mean  = atof(argv[2]);
    double sd    = atof(argv[3]);
    int    pause = atoi(argv[4]);
    int    N     = atoi(argv[5]);

    // shared memory
    size_t size = sizeof(Header) + N * sizeof(Slot);
    int fd = shm_open("/lab4_shm", O_CREAT | O_RDWR, 0666);
    ftruncate(fd, size);
    void* mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    Header* hdr  = (Header*)mem;
    Slot*   ring = (Slot*)(hdr + 1);

    // semaphores
    sem_t* lock      = sem_open("/lab4_lock", O_CREAT, 0666, 1);
    sem_t* freeSlots = sem_open("/lab4_free", O_CREAT, 0666, N);
    sem_t* usedSlots = sem_open("/lab4_used", O_CREAT, 0666, 0);

    default_random_engine gen(random_device{}());
    normal_distribution<double> dist(mean, sd);

    while (true) {
        double p = dist(gen);
        fprintf(stderr, "[%s] %s: generating a new value %.2f\n",
                stamp().c_str(), cname.c_str(), p);

        sem_wait(freeSlots);
        fprintf(stderr, "[%s] %s: trying to get mutex on shared buffer\n",
                stamp().c_str(), cname.c_str());
        sem_wait(lock);

        // critical section
        fprintf(stderr, "[%s] %s: placing %.2f on shared buffer\n",
                stamp().c_str(), cname.c_str(), p);
        strncpy(ring[hdr->head].name, cname.c_str(), 10);
        ring[hdr->head].name[10] = '\0';
        ring[hdr->head].price = p;
        hdr->head = (hdr->head + 1) % N;

        sem_post(lock);
        sem_post(usedSlots);

        fprintf(stderr, "[%s] %s: sleeping for %d ms\n",
                stamp().c_str(), cname.c_str(), pause);
        usleep(pause * 1000);
    }
    return 0;
}