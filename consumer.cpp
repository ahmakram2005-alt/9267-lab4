#include <cstdio>
#include <cstdlib>
#include <cstring>
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

const char* names[11] = {
    "ALUMINIUM", "COPPER", "COTTON", "CRUDEOIL", "GOLD", "LEAD",
    "MENTHAOIL", "NATURALGAS", "NICKEL", "SILVER", "ZINC"
};

double price[11], lastPrice[11];
double avg[11], lastAvg[11];
double last5[11][5];
int count[11];

void printValue(double now, double before) {
    if (now > before)
        printf("\033[32m%7.2lf\u2191\033[0m", now);
    else if (now < before)
        printf("\033[31m%7.2lf\u2193\033[0m", now);
    else
        printf("\033[34m%7.2lf \033[0m", now);
}

void showTable() {
    printf("\e[1;1H\e[2J");
    printf("+-------------------------------------+\n");
    printf("| Commodity  |   Price   |  AvgPrice  |\n");
    printf("+-------------------------------------+\n");

    for (int i = 0; i < 11; i++) {
        printf("| %-10s |  ", names[i]);
        printValue(price[i], lastPrice[i]);
        printf(" |  ");
        printValue(avg[i], lastAvg[i]);
        printf("  |\n");
    }

    printf("+-------------------------------------+\n");
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("usage: ./consumer N\n");
        return 1;
    }

    int N = atoi(argv[1]);

    
    shm_unlink("/lab4_shm");
    sem_unlink("/lab4_mutex");
    sem_unlink("/lab4_empty");
    sem_unlink("/lab4_full");

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

    showTable();

    while (true) {
        sem_wait(full);
        sem_wait(mutex);

        Item item = items[info->out];
        info->out = (info->out + 1) % N;

        sem_post(mutex);
        sem_post(empty);

        for (int i = 0; i < 11; i++) {
            if (strcmp(item.name, names[i]) == 0) {
                lastPrice[i] = price[i];
                lastAvg[i] = avg[i];
                price[i] = item.price;

                last5[i][count[i] % 5] = item.price;
                count[i]++;

                int n = count[i];
                if (n > 5) n = 5;

                double sum = 0;
                for (int k = 0; k < n; k++)
                    sum += last5[i][k];
                avg[i] = sum / n;
            }
        }

        showTable();
    }

    return 0;
}