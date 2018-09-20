#include "dispatchQueue.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sysinfo.h>
#include <unistd.h>


#define DEBUG 0                                                  
                                                                   
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
    task_t *newTask = malloc(sizeof (task_t));
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
    if (node->nodeTask) {
        task_destroy(node->nodeTask);
    }

    if (node->nextNode){
        free(node->nextNode);
    }

    free(node);

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

    DEBUG_PRINTLN("GOT PAST NULL CHECK\n");

    node_t *result = dispatchQueue->head;
    
    DEBUG_PRINTLN("RESULT ALLOCATION\n");
    
    // If there is a second node, set that to be the new head
    if (dispatchQueue->head->nextNode) {
        dispatchQueue->head = dispatchQueue->head->nextNode;
        DEBUG_PRINTLN("SET SECOND NODE TO BE HEAD\n");
        return result;
    } else {
        DEBUG_PRINTLN("SET HEAD TO BE NULL\n");
        // If not, set the new head of the dispatchQueue to be null
        dispatchQueue->head = NULL;
        return result;
    }
}

// Wrapper function to aid in blocking until queue has task to complete
void *wrapper_function(void* input)
{

    dispatch_queue_t *dispatchQueue = (dispatch_queue_t *)input;

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

        taskedNode->nodeTask->work(taskedNode->nodeTask->params);

        DEBUG_PRINTLN("EXECUTED TASK W/ NAME: ");
        //printf("%s\n", task->name);
        DEBUG_PRINTLN("END OF WHILE ITERATION\n");
        //task->work(task->params);

        //task_destroy(task);
        //node_destroy(taskedNode);
    }
}

dispatch_queue_t *dispatch_queue_create(queue_type_t queueType)
{

    DEBUG_PRINTLN("GOT SOMEWHERE\n");

    // Create new pointer to new dispatch queue, and allocate associated memory
    dispatch_queue_t *newDispatchQueue = malloc(sizeof(dispatch_queue_t));

    // Set the queue type field
    //newDispatchQueue->queue_type = malloc(sizeof(queue_type_t));
    DEBUG_PRINTLN("assigning queue type\n");
    newDispatchQueue->queue_type = queueType;
    DEBUG_PRINTLN("assigning head value\n");
    newDispatchQueue->head = NULL;
    
    // HAVE CHANGED, AS HEAD NODE IS FIRST CREATED BY PUSH
    // Allocate memory for the first task, that'll be set to point to the head of the list of tasks
    //newDispatchQueue->head = malloc(sizeof(node_t));

    int numberOfThreads = 0;

    if (queueType == CONCURRENT) 
    {
        // Get the number of cores of the machine
        numberOfThreads = sysconf(_SC_NPROCESSORS_ONLN);

        // Allocate space for the thread queue contained within the dispatch queue
        DEBUG_PRINTLN("assigning thread queue\n");
        newDispatchQueue->thread_queue = malloc(sizeof(dispatch_queue_thread_t)*numberOfThreads);
    } else if (queueType == SERIAL)
    {
        newDispatchQueue->thread_queue = malloc(sizeof(dispatch_queue_thread_t));
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
    node_t *current = head;

    while (current->nextNode) {
	node_t *next = current->nextNode;
	
	node_destroy(current);
	current = next;
    }

    DEBUG_PRINTLN("FREED NODES FROM LIST\n");
}

void dispatch_queue_destroy(dispatch_queue_t *dispatchQueue)
{
    DEBUG_PRINTLN("GOT INTO DISPATCH QUEUE DESTROY\n");

    if (dispatchQueue->head)
    {
        free_nodes_from_list(dispatchQueue->head);
    }
    
    // Free the thread queue field
    free(dispatchQueue->thread_queue);

    // Free the queue semaphore
    free(dispatchQueue->queue_semaphore);

    free(dispatchQueue);

    DEBUG_PRINTLN("DESTROYED DISPATCH QUEUE\n");
}

int dispatch_async(dispatch_queue_t *queue, task_t *task)
{
    // Push node onto end of queue
    node_t *pushedNode = malloc(sizeof(node_t));
    DEBUG_PRINTLN("pushing a node to the queue\n");
    push(queue, task);

    DEBUG_PRINTLN("posting to the semaphore\n");
    sem_post(queue->queue_semaphore);

    DEBUG_PRINTLN("ASYNC PUSHED A NODE\n");

}

int dispatch_sync(dispatch_queue_t *queue, task_t *task){
return 0;
}

void dispatch_for(dispatch_queue_t *queue, long num, void (*work)(long)){
}

int dispatch_queue_wait(dispatch_queue_t *queue){
return 0;
}
