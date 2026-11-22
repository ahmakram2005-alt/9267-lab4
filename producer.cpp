#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <random>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
using namespace std;

struct Item {
    char name[11];
    double price;
};

struct Info {
    int in;
    int out;
};

char* name;

void printLog(const char* msg) {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    tm* t = localtime(&ts.tv_sec);

    char date[30];
    strftime(date, 30, "%m/%d/%Y %H:%M:%S", t);
    fprintf(stderr, "[%s.%03ld] %s: %s\n", date, ts.tv_nsec / 1000000, name, msg);
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        printf("usage: ./producer NAME MEAN SD SLEEP_MS N\n");
        return 1;
    }

    name = argv[1];
    double mean = atof(argv[2]);
    double sd = atof(argv[3]);
    int T = atoi(argv[4]);
    int N = atoi(argv[5]);

    int size = sizeof(Info) + N * sizeof(Item);
    int fd = shm_open("/lab4_shm", O_CREAT | O_RDWR, 0666);
    ftruncate(fd, size);
    void* mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    Info* info = (Info*) mem;
    Item* items = (Item*) (info + 1);

    sem_t* mutex = sem_open("/lab4_mutex", O_CREAT, 0666, 1);
    sem_t* empty = sem_open("/lab4_empty", O_CREAT, 0666, N);
    sem_t* full  = sem_open("/lab4_full",  O_CREAT, 0666, 0);

    random_device rd;
    default_random_engine gen(rd());
    normal_distribution<double> dist(mean, sd);

    char msg[100];

    while (true) {
        double price = dist(gen);
        sprintf(msg, "generating a new value %.2f", price);
        printLog(msg);

        sem_wait(empty);
        printLog("trying to get mutex on shared buffer");
        sem_wait(mutex);

        sprintf(msg, "placing %.2f on shared buffer", price);
        printLog(msg);
        strcpy(items[info->in].name, name);
        items[info->in].price = price;
        info->in = (info->in + 1) % N;

        sem_post(mutex);
        sem_post(full);

        sprintf(msg, "sleeping for %d ms", T);
        printLog(msg);
        usleep(T * 1000);
    }

    return 0;
}