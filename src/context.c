#include <jobs.h>
#include <stdint.h>

context *context_init(int num_worker_threads, int max_job_per_thread) {
  context *context = malloc(sizeof(context));

  context->next_worker_id = 0;
  context->num_worker_threads = num_worker_threads;

  max_job_per_thread = nex_power_of_two_get(max_job_per_thread);
  context->max_jobs_per_thread = max_job_per_thread;

  context->worker_job_queues =
      malloc(sizeof(work_stealing_queue *) * num_worker_threads);
  const size_t jobPoolBufferSize =
      num_worker_threads * max_job_per_thread * sizeof(job) +
      JOB_CACHE_LINE_SIZE - 1;

  context->job_pool_buffer = malloc(jobPoolBufferSize);
  size_t queue_buffer_size = buffer_size_get(max_job_per_thread);
  context->queue_entry_buffer = malloc(queue_buffer_size * num_worker_threads);

  for (int worker_index = 0; worker_index < num_worker_threads;
       ++worker_index) {
    context->worker_job_queues[worker_index] = work_stealing_queue_init(
        max_job_per_thread,
        (void *)((intptr_t)context->queue_entry_buffer +
                 worker_index * queue_buffer_size),
        queue_buffer_size);
  }

  return context;
}

void context_destroy(context *context) {
  for (int worker_index = 0; worker_index < context->num_worker_threads;
       ++worker_index) {
    free(context->worker_job_queues[worker_index]);
  }

  free(context->worker_job_queues);
  free(context->queue_entry_buffer);
  free(context->job_pool_buffer);
}