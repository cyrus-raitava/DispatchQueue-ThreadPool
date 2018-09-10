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

void task_destroy(task_t *);

dispatch_queue_t *dispatch_queue_create(queue_type_t);

void dispatch_queue_destroy(dispatch_queue_t *);

int dispatch_async(dispatch_queue_t *, task_t *);

int dispatch_sync(dispatch_queue_t *, task_t *);

void dispatch_for(dispatch_queue_t *, long, void (*)(long));

int dispatch_queue_wait(dispatch_queue_t *);