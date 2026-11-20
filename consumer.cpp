#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
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

const int HOWMANY = 11;
const char* goods[HOWMANY] = {
    "ALUMINIUM", "COPPER", "COTTON", "CRUDEOIL", "GOLD", "LEAD",
    "MENTHAOIL", "NATURALGAS", "NICKEL", "SILVER", "ZINC"
};

const char* RED   = "\033[31m";
const char* GREEN = "\033[32m";
const char* CYAN  = "\033[36m";
const char* RESET = "\033[0m";

double last5[HOWMANY][5];
int    seen[HOWMANY];
double nowPrice[HOWMANY];
double oldPrice[HOWMANY];
double nowAvg[HOWMANY];
double oldAvg[HOWMANY];

// prints a number with color and arrow
void showNumber(double value, double before, int count) {
    const char* color = CYAN;
    const char* arrow = " ";
    if (count > 1) {
        if (value > before)      { color = GREEN; arrow = "\u2191"; }
        else if (value < before) { color = RED;   arrow = "\u2193"; }
    }
    printf("%s%7.2lf%s%s", color, value, arrow, RESET);
}

void drawTable() {
    printf("\033[1;1H\033[2J");   // clear screen
    printf("+-------------------------------------------+\n");
    printf("| Currency     |    Price     |   AvgPrice  |\n");
    printf("+-------------------------------------------+\n");
    for (int i = 0; i < HOWMANY; i++) {
        printf("| %-12s | ", goods[i]);
        showNumber(nowPrice[i], oldPrice[i], seen[i]);
        printf("     | ");
        showNumber(nowAvg[i], oldAvg[i], seen[i]);
        printf("    |\n");
    }
    printf("+-------------------------------------------+\n");
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s N\n", argv[0]);
        return 1;
    }
    int N = atoi(argv[1]);

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

    drawTable();

    while (true) {
        sem_wait(usedSlots);
        sem_wait(lock);

        // critical section: take one item out
        Slot item = ring[hdr->tail];
        hdr->tail = (hdr->tail + 1) % N;

        sem_post(lock);
        sem_post(freeSlots);

        // update the matching commodity
        for (int i = 0; i < HOWMANY; i++) {
            if (strcmp(item.name, goods[i]) == 0) {
                oldPrice[i] = nowPrice[i];
                oldAvg[i]   = nowAvg[i];

                nowPrice[i] = item.price;
                last5[i][seen[i] % 5] = item.price;
                seen[i]++;

                // average of the last 5 readings (or fewer at the start)
                int n = seen[i] < 5 ? seen[i] : 5;
                double sum = 0;
                for (int k = 0; k < n; k++) sum += last5[i][k];
                nowAvg[i] = sum / n;
                break;
            }
        }
        drawTable();
    }
    return 0;
}