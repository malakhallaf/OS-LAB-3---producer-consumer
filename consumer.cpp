#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <deque>

struct bufferItem {
    char name[11];
    double price;
};

struct sharedBuffer {
    int nextProdWrite;
    int consReadNext;
    sem_t mutex;
    sem_t isEmpty;
    sem_t isFull;
    bufferItem buffer[];
};

struct commodity {
    std::string name;
    double price = 0.0;
    double avgPrice = 0.0;
    std::deque<double> history;
    int priceTrend = 0;
    int avgTrend = 0;
};

void printDashboard(const commodity commodities[])
{
    printf("\e[1;1H\e[2J");
    printf("+-------------------------------------+\n");
    printf("| Currency      |  Price   | AvgPrice |\n");
    printf("+-------------------------------------+\n");

    for (int i = 0; i < 11; i++)
    {
        printf("| %-13s | ", commodities[i].name.c_str());

        if (commodities[i].priceTrend == 1)
            printf("\033[32m%7.2lf↑\033[0m", commodities[i].price);
        else if (commodities[i].priceTrend == -1)
            printf("\033[31m%7.2lf↓\033[0m", commodities[i].price);
        else
            printf("%7.2lf ", commodities[i].price);

        printf(" | ");

        if (commodities[i].avgTrend == 1)
            printf("\033[32m%7.2lf↑\033[0m", commodities[i].avgPrice);
        else if (commodities[i].avgTrend == -1)
            printf("\033[31m%7.2lf↓\033[0m", commodities[i].avgPrice);
        else
            printf("%7.2lf ", commodities[i].avgPrice);

        printf(" |\n");
    }

    printf("+-------------------------------------+\n");
    fflush(stdout);
}

int main(int argc, char* argv[])
{
if (argc != 2)
{
    fprintf(stderr, "USAGE: %s <buffer_size>\n", argv[0]);
    return 1;
}

int N = std::atoi(argv[1]);
if (N <= 0)
{
        fprintf(stderr, "%d is an invalid buffer size\n", N);
        return 1;
}

size_t shm_size = sizeof(sharedBuffer) + N * sizeof(bufferItem);

int fd = shm_open("/shared_buffer", O_CREAT|O_EXCL|O_RDWR, 0666);
bool created = false;
if (fd == -1)
{
    if (errno == EEXIST)
    {
        fd = shm_open("/shared_buffer", O_RDWR, 0666);
        if (fd == -1)
        {
            perror("shm_open");
            return 1;
        }
    }
    else
    {
        perror("shm_open");
        return 1;
    }
}
else
{
    created = true;
    if (ftruncate(fd, shm_size) == -1)
    {
        perror("ftruncate");
        return 1;
    }
}

sharedBuffer* shared = static_cast<sharedBuffer*>(mmap(NULL, shm_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0));
if (shared == MAP_FAILED)
{
    perror("mmap");
    return 1;
}
close(fd);

if (created)
{
    if (sem_init(&shared->mutex, 1, 1) == -1 || sem_init(&shared->isEmpty, 1, N) == -1 || sem_init(&shared->isFull, 1, 0) == -1)
    {
        perror("sem_init");
        return 1;
    }
}

commodity commodities[11] = {
    {"ALUMINIUM"}, {"COPPER"}, {"COTTON"}, {"CRUDEOIL"}, {"GOLD"}, {"LEAD"}, {"MENTHAOIL"}, {"NATURALGAS"}, {"NICKEL"}, {"SILVER"}, {"ZINC"}
};

printDashboard(commodities);

while (true)
{
    sem_wait(&shared->isFull);
    sem_wait(&shared->mutex);
    bufferItem item = shared->buffer[shared->consReadNext];
    shared->consReadNext = (shared->consReadNext + 1) % N;
    sem_post(&shared->mutex);
    sem_post(&shared->isEmpty);

    for (int i = 0; i < 11; i++)
    {
        if (commodities[i].name == item.name)
        {
            if (item.price > commodities[i].price)
            {
                commodities[i].priceTrend = 1;
            }
            else if (item.price < commodities[i].price)
            {
                commodities[i].priceTrend = -1;
            }
            else
            {
                commodities[i].priceTrend = 0;
            }

            commodities[i].price = item.price;

            commodities[i].history.push_back(item.price);
            if (commodities[i].history.size() > 5)
            {
                commodities[i].history.pop_front();
            }

            double sum = 0.0;
            for (double prices : commodities[i].history)
            {
                sum = sum + prices;
            }
            double newAvg = sum / commodities[i].history.size();

            if (newAvg > commodities[i].avgPrice)
            {
                commodities[i].avgTrend = 1;
            }
            else if (newAvg < commodities[i].avgPrice)
            {
                commodities[i].avgTrend = -1;
            }
            else
            {
                commodities[i].avgTrend = 0;
            }

            commodities[i].avgPrice = newAvg;

            break;
        }
    }
    printDashboard(commodities);
}
}