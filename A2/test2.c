/* 
 * File:   test2.c
 * Author: robert
 */

#include "dispatchQueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "dispatchQueue.c"

void test2() {
    sleep(1);
    printf("test2 running\n");
}

/*
 * Checks asynchronous dispatching.
 * The program should finish before the message from the test2 function is printed.
 * 
 */
int main(int argc, char** argv) {
    // create a concurrent dispatch queue
    dispatch_queue_t * concurrent_dispatch_queue;
    task_t *task;
    concurrent_dispatch_queue = dispatch_queue_create(CONCURRENT);
sleep(2);
    task = task_create(test2, NULL, "test2");
sleep(2);
    dispatch_async(concurrent_dispatch_queue, task);
    sleep(5);
    printf("Safely dispatched\n");
    //printf("NAME IS: %s", concurrent_dispatch_queue->head->nodeTask->name);
    dispatch_queue_destroy(concurrent_dispatch_queue);
    return EXIT_SUCCESS;
}
