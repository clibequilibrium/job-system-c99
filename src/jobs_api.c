#include <jobs.h>

static inline job *job_allocate() {
  // TODO(cort): no protection against over-allocation
  uint64_t index = tls_jobCount++;
  return &tls_jobPool[index & (tls_jobContext->max_jobs_per_thread - 1)];
}

static inline bool is_job_complete(const job *job) {
  return (job->unfinished_jobs == 0);
}

static void job_finish(job *job) {
  const int32_t unfinished_jobs = --(job->unfinished_jobs);
  assert(unfinished_jobs >= 0);
  if (unfinished_jobs == 0 && job->parent) {
    job_finish(job->parent);
  }
}

static job *job_get(void) {
  work_stealing_queue *queue = tls_jobContext->worker_job_queues[tls_workerId];
  job *job = work_stealing_queue_pop(queue);
  if (!job) {
    // this worker's queue is empty; try to steal a job from another thread
    int victim_offset = 1 + (rand() % tls_jobContext->num_worker_threads - 1);
    int victim_index =
        (tls_workerId + victim_offset) % tls_jobContext->num_worker_threads;
    work_stealing_queue *victim_queue =
        tls_jobContext->worker_job_queues[victim_index];
    job = work_stealing_queue_steal(victim_queue);
    if (!job) {    // nothing to steal
      _mm_pause(); // TODO(cort): busy-wait bad, right? But there might be a job
                   // to steal in ANOTHER queue, so we should try again shortly.
      return NULL;
    }
  }
  return job;
}

static inline void job_execute(job *job) {
  (job->function)(job, job->data);
  job_finish(job);
}

context *context_create(int num_workers, int max_jobs_per_worker) {
  return context_init(num_workers, max_jobs_per_worker);
}

int worker_init(context *ctx) {
  tls_jobContext = ctx;
  tls_jobCount = 0;
  tls_workerId = __atomic_fetch_add(&ctx->next_worker_id,1, __ATOMIC_SEQ_CST);
  assert(tls_workerId < ctx->num_worker_threads);
  void *jobPoolBufferAligned =
      (void *)((uintptr_t)ctx->job_pool_buffer + JOB_CACHE_LINE_SIZE - 1 &
               ~(JOB_CACHE_LINE_SIZE - 1));
  assert((uintptr_t)jobPoolBufferAligned % JOB_CACHE_LINE_SIZE == 0);
  tls_jobPool =
      (job *)(jobPoolBufferAligned) + tls_workerId * ctx->max_jobs_per_thread;
  return tls_workerId;
}

job *job_create(job_function function, job *parent, const void *embedded_data,
                size_t embedded_data_bytes) {
  if (embedded_data != NULL && embedded_data_bytes > JOB_CACHE_LINE_SIZE) {
    assert(0);
    return NULL;
  }
  if (parent) {
    parent->unfinished_jobs++;
  }
  job *job = job_allocate();
  job->function = function;
  job->parent = parent;
  job->unfinished_jobs = 1;
  if (embedded_data) {
    memcpy(job->padding, embedded_data, embedded_data_bytes);
    job->data = job->padding;
  } else {
    job->data = NULL;
  }
  return job;
}

int job_enqueue(job *job) {
  int pushError = work_stealing_queue_push(
      tls_jobContext->worker_job_queues[tls_workerId], job);
  return pushError;
}

void job_wait_for(const job *waiting_job) {
  while (!is_job_complete(waiting_job)) {
    job *nextJob = job_get();
    if (nextJob) {
      job_execute(nextJob);
    }
  }
}

int worker_get_id(void) { return tls_workerId; }