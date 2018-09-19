#include "dispatchQueue.h"
#include "num_cores.c"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define DEBUG 1                                                   
                                                                   
#if defined(DEBUG) && DEBUG > 0                                      
#define DEBUG_PRINTLN(fmt, args...)\
	printf("DEBUG: " fmt, ##args)                                   
#else                                                                
    #define DEBUG_PRINTLN(fmt, args...) //Do Nothing                     
#endif

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

    DEBUG_PRINTLN("CREATED TASK\n");

    // Return created task
    return newTask;
}

// Method to destroy the task structure
void task_destroy(task_t *task)
{
    free(task); 
    DEBUG_PRINTLN("DESTROYED TASK\n");
}

// Method to create node in singly-linked list
node_t *node_create(task_t *task, node_t *nextNode)
{
    node_t *newNode = malloc(sizeof (node_t));
    newNode->nodeTask = task;
    newNode->nextNode = nextNode;

    DEBUG_PRINTLN("===============\n");
    DEBUG_PRINTLN("CREATED NODE\n");
    DEBUG_PRINTLN("NODE HAS TASK NAME: %s\n", newNode->nodeTask->name);

    return newNode;
}


// Method to destroy a node
void node_destroy(node_t *node)
{
    task_destroy(node->nodeTask);

    if (node->nextNode){
        free(node->nextNode);
    }
    //free(node->prevNode);
    //free(node->nextNode);

    DEBUG_PRINTLN("DESTROYED NODE\n");
}

// Method to append task/node struct onto end of singly-linked list
void push(dispatch_queue_t *dispatchQueue, task_t *newTask)
{
    // Push node onto queue

    // Check if head exists, and if it doesn't, set
    // the new task, wrapped in a node, to be the head

    DEBUG_PRINTLN("PUSHING NODE W/ TASK NAME: %s\n", newTask->name);

    if (!dispatchQueue->head) {
        dispatchQueue->head = node_create(newTask, NULL);
        DEBUG_PRINTLN("HEAD DIDN'T EXIST, SO HEAD IS NOW TASK W/ NAME: %s\n", dispatchQueue->head->nodeTask->name);
        return;
    }

    // If the head does exist, follow the pointers of nextNode
    // down the singly-linked list, to find the current tail
    node_t *position = dispatchQueue->head;

    while (position->nextNode) {
        position = position->nextNode;
    }

    // Append task wrapped in node, onto tail of singly-linked list
    position->nextNode = node_create(newTask, NULL);
    return;
}

// Method to pop task/node struct off of beginning of singly-linked list
// (NOTE) it's important to remember to change the head pointer, when using this
node_t* pop(dispatch_queue_t *dispatchQueue)
{
    DEBUG_PRINTLN("GOT IN POP\n");
    
    if (!dispatchQueue->head) {
        DEBUG_PRINTLN("POP: HEAD WAS NULL, BAD BAD NOT GOOD\n");
        return NULL;
    }

    node_t *result = dispatchQueue->head;
    
    // If there is a second node, set that to be the new head
    if (dispatchQueue->head->nextNode) {
        dispatchQueue->head = dispatchQueue->head->nextNode;
        return result;
    } else {

        // If not, set the new head of the dispatchQueue to be null
        dispatchQueue->head = NULL;
        return NULL;
    }
}

// Wrapper function to aid in blocking until queue has task to complete
void *wrapper_function(void* input)
{

    dispatch_queue_t *dispatchQueue = (dispatch_queue_t *)input;

    //printf(dispatchQueue->head->nodeTask->name);

    while(1) 
    {
        // Wait until there is something on the queue to do
        sem_wait(dispatchQueue->queue_semaphore);

        DEBUG_PRINTLN("DOING TASK\n");

        // Pop the task off of the head
        node_t *taskedNode = pop(dispatchQueue);
        DEBUG_PRINTLN("POPPED NODE HAS NAME: %s\n", taskedNode->nodeTask->name);

        // Save the task to do
        task_t *task = taskedNode->nodeTask;

        printf("executing task with name %s\n", task->name);
        
        task->work(task->params);

        free(task);
        free(taskedNode);
    }
}

dispatch_queue_t *dispatch_queue_create(queue_type_t queueType)
{

    DEBUG_PRINTLN("GOT SOMEWHERE\n");

    // Create new pointer to new dispatch queue, and allocate associated memory
    dispatch_queue_t *newDispatchQueue = (dispatch_queue_t *)malloc(sizeof(dispatch_queue_t));

    // Set the queue type field
    //newDispatchQueue->queue_type = (queue_type_t *)malloc(sizeof(queue_type_t));
    DEBUG_PRINTLN("assigning queue type\n");
    newDispatchQueue->queue_type = queueType;
    DEBUG_PRINTLN("assigning head value\n");
    
    // HAVE CHANGED, AS HEAD NODE IS FIRST CREATED BY PUSH
    // Allocate memory for the first task, that'll be set to point to the head of the list of tasks
    //newDispatchQueue->head = (node_t *)malloc(sizeof(node_t));

    int numberOfThreads = 0;

    if (queueType = CONCURRENT) 
    {
        // Get the number of cores of the machine
        numberOfThreads = getNumberOfProcessors();

        // Allocate space for the thread queue contained within the dispatch queue
        DEBUG_PRINTLN("assigning thread queue\n");
        newDispatchQueue->thread_queue = (dispatch_queue_thread_t *)malloc(sizeof(dispatch_queue_thread_t)*numberOfThreads);
    } else if (queueType = SERIAL)
    {
        newDispatchQueue->thread_queue = (dispatch_queue_thread_t *)malloc(sizeof(dispatch_queue_thread_t));
        numberOfThreads = 1;
    }

    // Allocate memory for the semaphore to be used for the queue
    sem_t *semaphore = malloc(sizeof(sem_t));

    // Initialise the value of the semaphore
    // (initial value of 0, with the forked flag set to zero)
    sem_init(semaphore, 0, 0);
    DEBUG_PRINTLN("assigning queue semaphore\n");

    // Assign semaphore of queue
    newDispatchQueue->queue_semaphore = semaphore;

    // Iterate through threads in thread pool, and initialise each
    for (int i = 0; i < numberOfThreads; i++){
        pthread_t *threadPool = malloc(sizeof(pthread_t));
        dispatch_queue_thread_t *queue_pointer = &newDispatchQueue->thread_queue[i];
        DEBUG_PRINTLN("making a thread\n");
        pthread_create(threadPool, NULL, wrapper_function, newDispatchQueue);
    }

    // Return the newly made dispatch queue
    return newDispatchQueue;
}

// Free tasks in linked list, given the location of the head of the queue
void free_nodes_from_list(node_t *head)
{
    //node_t *next = (node_t *)malloc(sizeof (node_t));
    node_t* next = head;
    //node_t *toFree = (node_t *)malloc(sizeof (node_t));
    node_t* toFree;

    while (next->nextNode)
    {
        toFree = next;
        next = next->nextNode;
        node_destroy(toFree);
    }

    node_destroy(next);

    printf("FREED NODES FROM LIST\n");
}

void dispatch_queue_destroy(dispatch_queue_t *dispatchQueue)
{
    // NOTE THAT TASKS SHOULD PRESUMABLY BE FREE AT THIS POINT. MAKE A CHECK THAT THEY ARE

    if (!dispatchQueue->head)
    {
        free_nodes_from_list(dispatchQueue->head);
    }
    
    // Free the thread queue field
 //   free(dispatchQueue->thread_queue);

    // Free the queue type field
 //   free(dispatchQueue->queue_type);

    // Free the queue semaphore
 //   free(dispatchQueue->queue_semaphore);

    free(dispatchQueue);

    DEBUG_PRINTLN("DESTROYED DISPATCH QUEUE\n");
}

int dispatch_async(dispatch_queue_t *queue, task_t *task)
{
    // Push node onto end of queue
    node_t *pushedNode = (node_t *)malloc(sizeof(node_t));
    push(queue, task);

    sem_post(queue->queue_semaphore);

    DEBUG_PRINTLN("ASYNC PUSHED A NODE\n");

    // Make second element head of dispatch_queue

}

int dispatch_sync(dispatch_queue_t *, task_t *);

void dispatch_for(dispatch_queue_t *, long, void (*)(long));

int dispatch_queue_wait(dispatch_queue_t *);