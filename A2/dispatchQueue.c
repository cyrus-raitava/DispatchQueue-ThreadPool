#include "dispatchQueue.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sysinfo.h>
#include <unistd.h>


#define DEBUG 0                                                  

// Method to be used to print debug lines during execution:
// Set constant DEBUG to 0 to ignore print lines, and to 1 to print DEBUG lines                                              
#if defined(DEBUG) && DEBUG > 0                                      
#define DEBUG_PRINTLN(fmt, args...)\
	printf("DEBUG: " fmt, ##args)                                   
#else                                                                
    #define DEBUG_PRINTLN(fmt, args...) //Do Nothing                     
#endif

// Method to create a new task, from a series of parameters
task_t *task_create(void (*work)(void *), void *params, char *name) 
{
    DEBUG_PRINTLN("=========================\n");
    DEBUG_PRINTLN("CREATING NEW TASK\n");

    // Allocate memory for new task to be made
    task_t *newTask = malloc(sizeof (task_t));

    // Initialise (allocate memory for) and set the semaphore of the task
    sem_t *semaphore = malloc(sizeof(sem_t));
    sem_init(semaphore, 0, 0);
    newTask->taskSemaphore = semaphore;

    // Fill in the rest of the arguments for the new task, dependent
    // on input parameters to task_create() method
    newTask->params = params;
    newTask->work = work;
    strcpy(newTask->name, name);

    DEBUG_PRINTLN("CREATED TASK\n");

    // Return created task
    return newTask;
}

// Method to destroy the task structure
void task_destroy(task_t *task)
{
    DEBUG_PRINTLN("=========================\n");

    // Free memory in reverse order of allocation
    free(task->taskSemaphore);
    free(task); 
    DEBUG_PRINTLN("DESTROYED TASK\n");
}

// Method to create node in singly-linked list
node_t *node_create(task_t *task, node_t *nextNode)
{
    DEBUG_PRINTLN("=========================\n");

    // Allocate memory for the new node to be created
    node_t *newNode = malloc(sizeof (node_t));

    // Assign fields of new node struct accordingly
    newNode->nodeTask = task;
    newNode->nextNode = nextNode;

    DEBUG_PRINTLN("CREATED NODE\n");
    DEBUG_PRINTLN("NODE HAS TASK NAME: %s\n", newNode->nodeTask->name);

    // Return the newly created node
    return newNode;
}

// Method to destroy a node
void node_destroy(node_t *node)
{
    DEBUG_PRINTLN("=========================\n");

    //  If a task exists within the node, free it
    if (node->nodeTask) {
        task_destroy(node->nodeTask);
    }

    // Free the pointer pointing to the next node
    if (node->nextNode){
        free(node->nextNode);
    }

    // Free the node itself
    free(node);

    DEBUG_PRINTLN("DESTROYED NODE\n");
}

// Method to append task/node struct combination onto end of singly-linked list
void push(dispatch_queue_t *dispatchQueue, task_t *newTask)
{
    DEBUG_PRINTLN("=========================\n");

    // Lock the queue, prior to pushing anything onto it
    pthread_mutex_lock(dispatchQueue->lock);
    DEBUG_PRINTLN("LOCKED QUEUE NOW");

    DEBUG_PRINTLN("ATTEMPTING TO PUSH NODE W/ TASK NAME: %s\n", newTask->name);
    // If the queue's head doesn't exist, set it to be the newly made node
    if (!dispatchQueue->head) {
        dispatchQueue->head = node_create(newTask, NULL);
        DEBUG_PRINTLN("HEAD DIDN'T EXIST, SO HEAD IS NOW TASK W/ NAME: %s\n", dispatchQueue->head->nodeTask->name);
        
        // Unlock the queue, and return
        pthread_mutex_unlock(dispatchQueue->lock);
        return;
    }

    // If the head does exist, follow the pointers of nextNode
    // down the singly-linked list, to find the current tail
    node_t *position = dispatchQueue->head;
    while (position->nextNode) {
        position = position->nextNode;
    }

    // Append task wrapped in node, onto tail of singly-linked list
    // (note the nextNode pointer of the tail is set to NULL)
    position->nextNode = node_create(newTask, NULL);

    // Unlock the queue, and return
    pthread_mutex_unlock(dispatchQueue->lock);
    return;
}

// Method to pop task/node struct off of beginning of singly-linked list
node_t* pop(dispatch_queue_t *dispatchQueue)
{
    DEBUG_PRINTLN("=========================\n");

    // Locking queue, to pop off head
    pthread_mutex_lock(dispatchQueue->lock);

    // If the queue's head doesn't exist, unlock the queue and return NULL
    if (!dispatchQueue->head) {
        DEBUG_PRINTLN("ATTEMPTING TO POP TASK OFF QUEUE, BUT HEAD WAS NULL\n");
        pthread_mutex_unlock(dispatchQueue->lock);
        return NULL;
    }

    // Set the node to return, as the current head of the queue
    node_t *result = dispatchQueue->head;
    
    // If there are 2 or more nodes, set the second node to be the new head
    if (dispatchQueue->head->nextNode) {
        dispatchQueue->head = dispatchQueue->head->nextNode;
        DEBUG_PRINTLN("SET SECOND NODE TO BE HEAD\n");
        
        // Unlock the queue, and return result
        pthread_mutex_unlock(dispatchQueue->lock);
        return result;
    } else {
        DEBUG_PRINTLN("SET HEAD TO BE NULL\n");

        // If there is no second node, then you are popping off the last node,
        // and must set the head to now be null
        dispatchQueue->head = NULL;
        
        // Unlock queue and return result
        pthread_mutex_unlock(dispatchQueue->lock);
        return result;
    }
}

// Wrapper function to aid in blocking until queue has a task/tasks to complete
void *thread_function(void* input)
{
    // Cast input to expected type (dispatch_queue_t)
    dispatch_queue_t *dispatchQueue = (dispatch_queue_t *)input;

    while(1) {

        // Wait until there is something on the queue to do (a sem_post() called)
        sem_wait(dispatchQueue->queue_semaphore);

        DEBUG_PRINTLN("=========================\n");
        
        // Lock queue, and increment the number of executing threads field
        pthread_mutex_lock(dispatchQueue->lock);
	    dispatchQueue->numExecutingThreads++;
        pthread_mutex_unlock(dispatchQueue->lock);

        DEBUG_PRINTLN("DOING TASK\n");

        // Pop the task off of the head of the queue
        // (note locking of queue occurs within pop() method)
        node_t *taskedNode = pop(dispatchQueue);
        DEBUG_PRINTLN("POPPED NODE HAS NAME: %s\n", taskedNode->nodeTask->name);

        // Save the task to execute
        task_t *task = taskedNode->nodeTask;

        // Execute work function of task
        taskedNode->nodeTask->work(taskedNode->nodeTask->params);
        
        // Use sem_post() to let dispatch_sync() method know execution of task has completed
        sem_post(task->taskSemaphore);

        DEBUG_PRINTLN("EXECUTED TASK W/ NAME: ");

        // Lock queue, and decrement the number of executing threads field
        pthread_mutex_lock(dispatchQueue->lock);
	    dispatchQueue->numExecutingThreads--;
        pthread_mutex_unlock(dispatchQueue->lock);
    }
}

// Method to create dispatch_queue_t struct, given a queue type
dispatch_queue_t *dispatch_queue_create(queue_type_t queueType)
{
    DEBUG_PRINTLN("=========================\n");
    DEBUG_PRINTLN("CREATING DISPATCH QUEUE\n");

    // Create new pointer to new dispatch queue, and allocate associated memory
    dispatch_queue_t *newDispatchQueue = malloc(sizeof(dispatch_queue_t));

    // Set the queue_type, and initial head of the newly made dispatchQueue
    newDispatchQueue->queue_type = queueType;
    newDispatchQueue->head = NULL;

    // Set number of executing threads to (initially) be zero
    newDispatchQueue->numExecutingThreads = 0;

    // Allocate memory for lock
    newDispatchQueue->lock = malloc(sizeof(pthread_mutex_t));

    // Create dispatch queue lock CHECK IF RETURN ZERO OR FAIL
    if (!(pthread_mutex_init(newDispatchQueue->lock, NULL) == 0)) {
        perror("ERROR: QUEUE LOCK COULD NOT BE INITIALISED");
    }

    // Allocate memory for the semaphore to be used for the queue
    sem_t *semaphore = malloc(sizeof(sem_t));

    // Initialise the value of the semaphore
    // (initial value of 0, with the forked flag set to zero)
    sem_init(semaphore, 0, 0);
    DEBUG_PRINTLN("assigning queue semaphore\n");

    // Assign semaphore of queue
    newDispatchQueue->queue_semaphore = semaphore;

    // Depending on type of queue, initialise the number of threads to spawn accordingly
    int numberOfThreads = 0;
    if (queueType == CONCURRENT) {
        // Get the number of cores of the machine (and thus the number of threads to spawn)
        numberOfThreads = sysconf(_SC_NPROCESSORS_ONLN);

        // Allocate space for the thread queue contained within the dispatch queue
        DEBUG_PRINTLN("assigning thread queue\n");
        newDispatchQueue->thread_queue = malloc(sizeof(dispatch_queue_thread_t)*numberOfThreads);

    } else if (queueType == SERIAL) {
        newDispatchQueue->thread_queue = malloc(sizeof(dispatch_queue_thread_t));
        numberOfThreads = 1;
    }

    // Iterate through threads in thread pool, and initialise each, with 
    // the threads themselves executing the thread_function()
    for (int i = 0; i < numberOfThreads; i++){

        // Create pointer to new position in thread pool (to later assign to)
        dispatch_queue_thread_t *queue_pointer = &newDispatchQueue->thread_queue[i];

        DEBUG_PRINTLN("MAKING A THREAD\n");

        // Create thread 
        pthread_create(&queue_pointer->thread, NULL, thread_function, newDispatchQueue);
    }

    // Return the newly made dispatch queue
    return newDispatchQueue;
}

// Free tasks in singly-linked list, given the location of the head of the queue
void free_nodes_from_list(node_t *head)
{
    DEBUG_PRINTLN("=========================\n");
    DEBUG_PRINTLN("FREEING NODES FROM LIST\n");
    // Note current position as that at the head of the queue
    node_t *current = head;

    // Iterate through nodes from the head, destroying the nodes as you traverse
    // until you hit the tail
    while (current->nextNode) {
        // Increment node position
        node_t *next = current->nextNode;
        
        // Destroy the current node, and move forward
        node_destroy(current);
        current = next;
    }

    DEBUG_PRINTLN("FREED NODES FROM LIST\n");
}

void dispatch_queue_destroy(dispatch_queue_t *dispatchQueue)
{
    DEBUG_PRINTLN("=========================\n");
    DEBUG_PRINTLN("DESTROYING DISPATCH QUEUE\n");

    // Check if the head of the dispatch queue exists, and if it 
    //does, destroy the list of tasks
    if (dispatchQueue->head) {
        free_nodes_from_list(dispatchQueue->head);
    }
    
    // Free the thread queue field, queue semaphore, lock, and finally dispatchQueue field
    free(dispatchQueue->thread_queue);
    free(dispatchQueue->queue_semaphore);
    free(dispatchQueue->lock);
    free(dispatchQueue);

    DEBUG_PRINTLN("DESTROYED DISPATCH QUEUE\n");
}

// Method to asynchronously dispatch a task to the queue
int dispatch_async(dispatch_queue_t *queue, task_t *task)
{
    DEBUG_PRINTLN("=========================\n");
    DEBUG_PRINTLN("BEGINNING ASYNC DISPATCH")

    // Push node onto end of queue
    push(queue, task);

    DEBUG_PRINTLN("POSTING TO SEMAPHORE, THAT TASK HAS BEEN QUEUED\n");

    // Post to the semaphore, that a task has been queued
    sem_post(queue->queue_semaphore);

    DEBUG_PRINTLN("ASYNC FINISHED PUSHING A NODE\n");
    return 0;
}

// Method to synchronously dispatch task onto queue
int dispatch_sync(dispatch_queue_t *queue, task_t *task)
{
    DEBUG_PRINTLN("=========================\n");
    DEBUG_PRINTLN("BEGINNING SYNC DISPATCH")

    // Push task onto queue
    push(queue, task);

    // Let all threads waiting on queue, know that there is another task on the queue
    sem_post(queue->queue_semaphore);

    // Wait until the task itself has been completed, to return
    sem_wait(task->taskSemaphore);    
    return 0;
}

// Method to dispatch a number of tasks, with certain parameters, onto the queue
void dispatch_for(dispatch_queue_t *queue, long num, void (*work)(long))
{
    DEBUG_PRINTLN("=========================\n");
    DEBUG_PRINTLN("BEGINNING DISPATCH FOR")

    // Use for-loop to create a 'num' number of tasks, to which you dispatch
    // all to the same queue
    for (long i = 0; i < num; i++) {
        long number = i;

        // Create task, which wraps the function/parameters
        task_t *task = malloc(sizeof(task_t));
        task = task_create((void(*)(void *))work, (void *)number, "task");
        
        // Push the task onto the queue asynchronously, to loop as quickly as possible
        dispatch_async(queue, task);
    }

    // Wait until the queue is empty, to destroy, return and exit
    dispatch_queue_wait(queue);
    dispatch_queue_destroy(queue);
}

// Method to wait until all tasks have been dispatched off of the queue, and 
int dispatch_queue_wait(dispatch_queue_t *queue){
    DEBUG_PRINTLN("=========================\n");
    DEBUG_PRINTLN("BEGINNING QUEUE WAIT")
    
	// Only return when number of threads executing is NONE, and the head of the queue is NULL (queue is empty)
	while (1) {
		DEBUG_PRINTLN("LOOKING AT NUMEXECUTINGTHREADS: %d\t", queue->numExecutingThreads);
		DEBUG_PRINTLN("QUEUE HEAD IS: %s\n", queue->head->nodeTask->name);
        if ((!queue->head) && (queue->numExecutingThreads == 0)) {
			return 0;
		}
	}

}
