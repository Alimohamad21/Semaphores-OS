#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>

const int N = 8, B = 5;

typedef struct Node {
    int data;
    struct Node *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    int noOfItems;
} Queue;

Node *newNode(int val);

Queue *init();

int isEmpty(Queue *q);

int isFull(Queue *q);

int dequeue(Queue *q);

void enqueue(Queue *q, int val);

void *collector();

void *monitor();

void *counter();

Queue *buffer;
int mCounter = 0;
sem_t counterSem, bufferSem, checkFullSem, checkEmptySem;

int main() {
    buffer = init();
    pthread_t mCounterTh[N], mMonitorTh, mCollectorTh;
    sem_init(&counterSem, 0, 1);
    sem_init(&bufferSem, 0, 1);
    sem_init(&checkFullSem, 0, B);
    sem_init(&checkEmptySem, 0, 0);
    for (int i = 0; i < N; i++) {
        pthread_create(&mCounterTh[i], NULL, counter, NULL);
    }
    pthread_create(&mMonitorTh, NULL, monitor, NULL);
    pthread_create(&mCollectorTh, NULL, collector, NULL);
    for (int i = 0; i < N; i++) {
        pthread_join(mCounterTh[i], NULL);
    }
    pthread_join(mMonitorTh, NULL);
    pthread_join(mCollectorTh, NULL);
}

void *counter() {
    time_t t;
    srand((unsigned) time(&t));
    pthread_t threadId = pthread_self();
    while (1) {
        sleep(5 + rand() % 5);
        printf("Counter thread %d: received a message\n", threadId);
        printf("Counter thread %d: waiting to write\n", threadId);
        sem_wait(&counterSem);
        printf("Counter thread %d: now adding to counter,counter value=%d\n", threadId, ++mCounter);
        sem_post(&counterSem);
    }

}

void *monitor() {
    time_t t;
    srand((unsigned) time(&t));
    int temp, checkFullSemValue, isFull;
    while (1) {
        sleep(8 + rand() % 9);
        printf("Monitor thread: waiting to read counter\n");
        sem_wait(&counterSem);
        printf(" Monitor thread: reading a count value of %d\n", mCounter);
        temp = mCounter;      // to save old counter value before resetting it for enqueue
        mCounter = 0;
        sem_post(&counterSem);
        sem_getvalue(&checkFullSem, &checkFullSemValue);
        isFull = checkFullSemValue < 0 ? 1 : 0;
        if (isFull)
            printf("Monitor thread: Buffer full!!\n");
        sem_wait(&checkFullSem);
        sem_wait(&bufferSem);
        enqueue(buffer, temp);
        printf("Monitor thread: writing to buffer at position %d\n", buffer->noOfItems - 1);
        sem_post(&bufferSem);
        sem_post(&checkEmptySem);
    }

}

void *collector() {
    time_t t;
    srand((unsigned) time(&t));
    int checkEmptySemValue, isEmpty;
    while (1) {
        sleep(10 + rand() % 10);
        sem_getvalue(&checkEmptySem, &checkEmptySemValue);
        isEmpty = checkEmptySemValue <= 0 ? 1 : 0;
        if (isEmpty)
            printf("Collector thread: nothing is in the buffer!\n");
        sem_wait(&checkEmptySem);
        sem_wait(&bufferSem);
        printf("Collector thread: reading from the buffer at position 0\n");
        dequeue(buffer);
        sem_post(&bufferSem);
        sem_post(&checkFullSem);
    }

}

Node *newNode(int val) {
    Node *n = malloc(sizeof(Node));
    n->data = val;
    n->next = NULL;
    return n;
}

Queue *init() {
    Queue *q = malloc(sizeof(Queue));
    q->head = q->tail = NULL;
    q->noOfItems = 0;
    return q;
}

int isEmpty(Queue *q) {
    if (!q->noOfItems)
        return 1;
    return 0;
}

int isFull(Queue *q) {
    if (q->noOfItems == B)
        return 1;
    return 0;
}

int dequeue(Queue *q) {

    if (q->head) {
        int val = q->head->data;
        Node *temp = q->head;
        q->head = q->head->next;
        free(temp);
        q->noOfItems--;
        if (q->head == NULL)
            q->tail = NULL;
        return val;
    }
    return -1;
}

void enqueue(Queue *q, int val) {
    Node *n = newNode(val);
    if (!q->head)
        q->head = q->tail = n;
    else {
        q->tail->next = n;
        q->tail = n;
    }
    q->noOfItems++;
}
