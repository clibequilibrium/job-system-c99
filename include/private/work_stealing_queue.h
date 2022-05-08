#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "api_types.h"

typedef struct work_stealing_queue {
  job **entries;
  atomic_uint top;
  atomic_uint bottom;
  int capacity;
} work_stealing_queue;

size_t buffer_size_get(int capacity);
extern work_stealing_queue *work_stealing_queue_init(int capacity, void *buffer,
                                                     size_t buffer_size);

extern int work_stealing_queue_push(work_stealing_queue *queue, job *job);
extern job *work_stealing_queue_pop(work_stealing_queue *queue);
extern job *work_stealing_queue_steal(work_stealing_queue *queue);