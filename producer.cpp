#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <ctime>
#include <cstring>

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

void printDateTime(const char* commodity)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm* t = localtime(&ts.tv_sec);
    long ms = ts.tv_nsec / 1000000;

    fprintf(stderr, "[%02d/%02d/%04d %02d:%02d:%02d.%03ld] %s: ",
            t->tm_mon + 1, t->tm_mday, t->tm_year + 1900,
            t->tm_hour, t->tm_min, t->tm_sec, ms, commodity);
}

int main(int argc, char* argv[])
{
if (argc != 6)
{
    fprintf(stderr, "USAGE: %s <name> <mean> <stddev> <sleep_ms> <buffer_size>\n", argv[0]);
    return 1;
}

const char* commodity = argv[1];
double mean = std::atof(argv[2]);
double stddev = std::atof(argv[3]);
int sleep_ms = std::atoi(argv[4]);
int N = std::atoi(argv[5]);

if (N <= 0 || sleep_ms < 0 || stddev < 0)
{
    fprintf(stderr, "invalid arguments\n");
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

std::default_random_engine generator(std::random_device{}());
std::normal_distribution<double> distribution(mean, stddev);

while (true)
{
    double price = distribution(generator);

    printDateTime(commodity);
    fprintf(stderr, "generating a new value %.2lf\n", price);

    printDateTime(commodity);
    fprintf(stderr, "trying to get mutex on shared buffer\n");

    sem_wait(&shared->isEmpty);
    sem_wait(&shared->mutex);

    printDateTime(commodity);
    fprintf(stderr, "placing %.2lf on shared buffer\n", price);

    std::strncpy(shared->buffer[shared->nextProdWrite].name, commodity, 10);
    shared->buffer[shared->nextProdWrite].name[10] = '\0';
    shared->buffer[shared->nextProdWrite].price = price;
    shared->nextProdWrite = (shared->nextProdWrite + 1) % N;

    sem_post(&shared->mutex);
    sem_post(&shared->isFull);

    printDateTime(commodity);
    fprintf(stderr, "sleeping for %d ms\n", sleep_ms);

    usleep(sleep_ms * 1000);
}
}