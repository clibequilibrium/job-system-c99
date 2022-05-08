#ifndef JOBS_API_H
#define JOBS_API_H

#include <assert.h>
#include <pthread.h>
#include <immintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "api_types.h"
#include "jobs.h"

static __thread context *tls_jobContext = NULL;
static __thread uint64_t tls_jobCount = 0;
static __thread int tls_workerId = -1;
static __thread job *tls_jobPool = NULL;

static inline job *job_allocate();
static inline bool is_job_complete(const job *job);

static void job_finish(job *job);
static job *job_get(void);
static inline void job_execute(job *job);

context *context_create(int num_workers, int max_jobs_per_worker);
int worker_init(context *ctx);

job *job_create(job_function function, job *parent, const void *embedded_data,
                size_t embedded_data_bytes);

int job_enqueue(job *job);
void job_wait_for(const job *job);
int worker_get_id(void);

#endif