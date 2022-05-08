#ifndef JOBS_API_TYPES_H
#define JOBS_API_TYPES_H

#include <assert.h>
#include <stdint.h>
#include "api_defines.h"

typedef void (*job_function)(const void *, const void *);
typedef struct job {
  job_function function;
  struct job *parent;
  void *data;
  int_fast32_t unfinished_jobs;
  char padding[JOB_PADDING_BYTES];
} job;

_Static_assert((sizeof(struct job) % JOB_CACHE_LINE_SIZE) == 0,
               "Job struct is not cache-line-aligned");

#endif