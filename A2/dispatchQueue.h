/* 
 * File:   dispatchQueue.h
 * Author: robert
 *
 * Modified by: crai897
 */

#ifndef DISPATCHQUEUE_H
#define	DISPATCHQUEUE_H

#include <pthread.h>
#include <semaphore.h>
    
#define error_exit(MESSAGE)     perror(MESSAGE), exit(EXIT_FAILURE)

    typedef enum { // whether dispatching a task synchronously or asynchronously
        ASYNC, SYNC
    } task_dispatch_type_t;
    
    typedef enum { // The type of dispatch queue.
        CONCURRENT, SERIAL
    } queue_type_t;

    // Task structure
    typedef struct task {
        char name[64];              // to identify it when debugging
        void (*work)(void *);       // the function to perform
        void *params;               // parameters to pass to the function
        task_dispatch_type_t type;  // asynchronous or synchronous
    } task_t;

    typedef struct node node_t;
    
    // Struct to be used for singly-linked list of tasks
    struct node {
        task_t *nodeTask;
        node_t *nextNode;
    };
    
    typedef struct dispatch_queue_t dispatch_queue_t; // the dispatch queue type
    typedef struct dispatch_queue_thread_t dispatch_queue_thread_t; // the dispatch queue thread type

    struct dispatch_queue_thread_t {
        dispatch_queue_t *queue;// the queue this thread is associated with
        pthread_t thread;       // the thread which runs the task
        sem_t thread_semaphore; // the semaphore the thread waits on until a task is allocated
        task_t *task;           // the current task for this thread
    };

    struct dispatch_queue_t {
        // the type of queue - serial or concurrent
        queue_type_t queue_type; 

        // Pointer to first node in linked list tasks array
        node_t *head;

        // Pointer to element of list of threads (order is irrelevant)
        dispatch_queue_thread_t *thread_queue;

        // Semaphore to let threads know when queue has a task on it
        sem_t *queue_semaphore;

	volatile int numExecutingThreads;
    };
   
    task_t *task_create(void (*)(void *), void *, char*);
    
    void task_destroy(task_t *);

    dispatch_queue_t *dispatch_queue_create(queue_type_t);
    
    void dispatch_queue_destroy(dispatch_queue_t *);
    
    int dispatch_async(dispatch_queue_t *, task_t *);
    
    int dispatch_sync(dispatch_queue_t *, task_t *);
    
    void dispatch_for(dispatch_queue_t *, long, void (*)(long));
    
    int dispatch_queue_wait(dispatch_queue_t *);

#endif	/* DISPATCHQUEUE_H */
