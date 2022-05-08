#ifndef JOBS_API_DEFINES_H
#define JOBS_API_DEFINES_H

#include "api_types.h"

#define JOB_CACHE_LINE_SIZE 64
#define JOB_PADDING_BYTES                                                      \
  ((JOB_CACHE_LINE_SIZE) - (sizeof(job_function) + sizeof(struct job *) +      \
                            sizeof(void *) + sizeof(int_fast32_t)))

#endif