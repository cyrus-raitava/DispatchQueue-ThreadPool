#include "dispatchQueue.h"
#include "num_cores.c"
#include "string.h"

// Method to create a new task, from a series of parameters
task_t *task_create(void (*work)(void *), void *params, char *name) 
{
    // Create new reference to task to be made
    task_t *newTask;

    // Allocate memory for the new task
    newTask = malloc(sizeof(task_t));

    // Fill in new task parameters from method declaration
    strcpy(newTask->name, name);

    // Fill in the rest of the arguments for the new task
    newTask->params = params;
    newTask->work = work;

    // Return created task
    return newTask;
}

// Method to destroy the task structure
void task_destroy(task_t *task)
{
    free(task); 
}

dispatch_queue_t *dispatch_queue_create(queue_type_t queueType)
{
    // Create new pointer to new dispatch queue
    dispatch_queue_t *newDispatchQueue;
    
    // Allocate memory for dispatch queue
    newDispatchQueue = malloc(sizeof(dispatch_queue_t));

    // Set the queue type field
    newDispatchQueue->queue_type = queueType;

    // Allocate memory for the first task, that'll be set to point to the head of the list of tasks
    newDispatchQueue->head = (task_t*)(malloc(sizeof(task_t)));

    // Get the number of cores of the computer
    int numberOfThreads = num_cores();

    // Allocate space for the thread queue contained within the dispatch queue
    newDispatchQueue->threadQueue = (dispatch_queue_thread_t*)(malloc(sizeof(dispatch_queue_thread_t)*numberOfThreads));

    // Return the newly made dispatch queue
    return newDispatchQueue;
}

void dispatch_queue_destroy(dispatch_queue_t *dispatchQueue)
{
    // NOTE THAT TASKS SHOULD PRESUMABLY BE FREE AT THIS POINT. MAKE A CHECK THAT THEY ARE
    
    // Free the thread queue field
    free(dispatchQueue->threadQueue);

}

int dispatch_async(dispatch_queue_t *, task_t *);

int dispatch_sync(dispatch_queue_t *, task_t *);

void dispatch_for(dispatch_queue_t *, long, void (*)(long));

int dispatch_queue_wait(dispatch_queue_t *);