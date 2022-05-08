#ifndef JOBS_CONTEXT_H
#define JOBS_CONTEXT_H

#include "api_types.h"
#include "jobs.h"

typedef struct context {
  work_stealing_queue **worker_job_queues;
  void *job_pool_buffer;
  void *queue_entry_buffer;
  atomic_int next_worker_id;
  int num_worker_threads;
  int max_jobs_per_thread;
} context;

extern context *context_init(int num_worker_threads, int max_job_per_thread);
extern void context_destroy(context *context);

static inline uint32_t nex_power_of_two_get(uint32_t x) {
  x = x - 1;
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x + 1;
}

#endif