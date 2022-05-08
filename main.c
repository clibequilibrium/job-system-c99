
#include <jobs.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#define NUM_WORKERS_THREADS 8
#define TOTAL_JOB_COUNT (128 * 1024)
#define BILLION 1000000000L

static const int MAX_JOBS_PER_THREAD = (TOTAL_JOB_COUNT / NUM_WORKERS_THREADS);
static int_fast32_t finished_job_count = 0;

static job_function empty_job(job *job, const void *data) {
  (void)job;
  (void)data;
  __atomic_fetch_add(&finished_job_count, 1, __ATOMIC_SEQ_CST);
}

static void *empty_worker_test(void *ctx) {
  int worker_id = worker_init(ctx);

  const int job_count = ((context *)ctx)->max_jobs_per_thread;
  int jobId = (worker_id << 16) | 0;
  job *root = job_create(empty_job, NULL, &jobId, sizeof(int));
  job_enqueue(root);
  for (int job_id = 1; job_id < job_count; job_id += 1) {
    int jobId = (worker_id << 16) | job_id;
    job *job = job_create(empty_job, root, &jobId, sizeof(int));
    int error = job_enqueue(job);
    assert(!error);
  }
  job_wait_for(root);
}

int main(void) {
  {
    context *context = context_create(NUM_WORKERS_THREADS, MAX_JOBS_PER_THREAD);

    struct timespec start, end;
    uint64_t elapsed;

    clock_gettime(CLOCK_MONOTONIC, &start);

    pthread_t workers[NUM_WORKERS_THREADS];

    for (int thread_id = 0; thread_id < NUM_WORKERS_THREADS; thread_id++) {
      pthread_create(&workers[thread_id], NULL, empty_worker_test, context);
    }

    for (int thread_id = 0; thread_id < NUM_WORKERS_THREADS; thread_id++) {

      pthread_join(workers[thread_id], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed =
        BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;

    printf("%d jobs complete in %llu nanoseconds\n", (int)finished_job_count,
           (long long unsigned int)elapsed);

    context_destroy(context);
  }

  getchar();
}