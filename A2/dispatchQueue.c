#include "dispatchQueue.h"
#include "num_cores.c"

task_t *task_create(void (*)(void *), void *, char *);

void task_destroy(task_t *);

dispatch_queue_t *dispatch_queue_create(queue_type_t);

void dispatch_queue_destroy(dispatch_queue_t *);

int dispatch_async(dispatch_queue_t *, task_t *);

int dispatch_sync(dispatch_queue_t *, task_t *);

void dispatch_for(dispatch_queue_t *, long, void (*)(long));

int dispatch_queue_wait(dispatch_queue_t *);