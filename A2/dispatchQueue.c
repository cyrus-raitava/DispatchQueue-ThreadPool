#include "dispatchQueue.h"
#include "num_cores.c"
#include "string.h"

// Method to create a new task, from a series of parameters
task_t *task_create(void (*work)(void *), void *params, char *name) 
{
    // Create new reference to task to be made
    task_t *newTask = (task_t *)malloc(sizeof (task_t));
    // Fill in the rest of the arguments for the new task
    newTask->params = params;
    newTask->work = work;
    // Fill in new task parameters from method declaration
    strcpy(newTask->name, name);
    // Return created task
    return newTask;
}

// Method to destroy the task structure
void task_destroy(task_t *task)
{
    free(task); 
}

// Method to create node in doubly-linked list
node_t *node_create(task_t *task, node_t *prevNode, node_t *nextNode)
{
    node_t *newNode = (node_t *)malloc(sizeof (node_t));
    newNode->nodeTask = task;
    newNode->prevNode = prevNode;
    newNode->nextNode = nextNode;

    return newNode;
}

// Method to destroy a node
void node_destroy(node_t *node)
{
    task_destroy(node->nodeTask);
    free(node->prevNode);
    free(node->nextNode);
}

// Method to append task/node struct onto end of doubly-linked list
void append(node_t *head, task_t *newTask)
{
    node_t * current = head;
    while (current->nextNode != NULL) {
        current = current->nextNode;
    }

    current->nextNode = node_create(newTask, current, NULL);
}

// Method to prepend task/node struct onto beginning of doubly-linked list
// (NOTE) it's important to remember to change the head pointer, when using this
void prepend(node_t *head, task_t *newTask)
{
    head->prevNode = node_create(newTask, NULL, head);
}

dispatch_queue_t *dispatch_queue_create(queue_type_t queueType)
{
    // Create new pointer to new dispatch queue, and allocate associated memory
    dispatch_queue_t *newDispatchQueue = (dispatch_queue_t *)malloc(sizeof(dispatch_queue_t));

    // Set the queue type field
    newDispatchQueue->queue_type = queueType;

    // Allocate memory for the first task, that'll be set to point to the head of the list of tasks
    newDispatchQueue->head = (node_t*)(malloc(sizeof(node_t)));

    if (queueType = CONCURRENT) 
    {
        // Get the number of cores of the computer
        int numberOfThreads = num_cores();

        // Allocate space for the thread queue contained within the dispatch queue
        newDispatchQueue->threadQueue = (dispatch_queue_thread_t *)malloc(sizeof(dispatch_queue_thread_t)*numberOfThreads);
    } else if (queueType = SERIAL)
    {
        newDispatchQueue->threadQueue = (dispatch_queue_thread_t *)malloc(sizeof(dispatch_queue_thread_t));
    }


    // Return the newly made dispatch queue
    return newDispatchQueue;
}

void dispatch_queue_destroy(dispatch_queue_t *dispatchQueue)
{
    // NOTE THAT TASKS SHOULD PRESUMABLY BE FREE AT THIS POINT. MAKE A CHECK THAT THEY ARE

    if (dispatchQueue->head != NULL)
    {
        free_node_from_list(dispatchQueue->head);
    }
    
    // Free the thread queue field
    free(dispatchQueue->threadQueue);

    // Free the queue type field
    free(dispatchQueue->queue_type);
}

// Free tasks in linked list, given the location of the head of the queue
void free_nodes_from_list(node_t *head)
{
    node_t *next = (node_t *)malloc(sizeof (node_t));
    next = &head;
    node_t *toFree = (node_t *)malloc(sizeof (node_t));

    while (next->nextNode != NULL)
    {
        toFree = next;
        next = next->nextNode;
        task_destroy(toFree);
    }

    node_destroy(next);
    node_destroy(toFree);
}

void wrapper_function(task_t *task)
{
    sem_t firstSem;
    sem_init(&firstSem, 0, 10);
}

int dispatch_async(dispatch_queue_t *queue, task_t *task)
{

    // Prepend task to start of queue
    prepend(queue->head, task);

    // Make second element head of dispatch_queue

}

int dispatch_sync(dispatch_queue_t *, task_t *);

void dispatch_for(dispatch_queue_t *, long, void (*)(long));

int dispatch_queue_wait(dispatch_queue_t *);