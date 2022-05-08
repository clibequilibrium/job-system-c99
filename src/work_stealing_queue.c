#include <assert.h>
#include <jobs.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

size_t buffer_size_get(int capacity) { return capacity * sizeof(job *); }

work_stealing_queue *work_stealing_queue_init(int capacity, void *buffer,
                                              size_t buffer_size) {
  work_stealing_queue *queue = malloc(sizeof(work_stealing_queue));
  if ((capacity & (capacity - 1)) != 0) {
    perror("Capacity must be a power of 2");
    return NULL;
  }

  size_t min_buffer_size = buffer_size_get(capacity);
  if (buffer_size < min_buffer_size) {
    perror("Inadequate buffer size");
    return NULL;
  }
  uint8_t *buffer_next = (uint8_t *)buffer;
  queue->entries = (job **)buffer_next;
  buffer_next += capacity * sizeof(job *);
  assert(buffer_next - (uint8_t *)buffer == (intptr_t)min_buffer_size);

  for (int entry = 0; entry < capacity; entry += 1) {
    queue->entries[entry] = NULL;
  }

  atomic_store(&queue->top, 0);
  atomic_store(&queue->bottom, 0);
  queue->capacity = capacity;

  return queue;
}

int work_stealing_queue_push(work_stealing_queue *queue, job *job) {
  uint32_t job_index =
      atomic_load_explicit(&queue->bottom, memory_order_relaxed);
  atomic_store_explicit(&queue->entries[job_index & (queue->capacity - 1)], job,
                        memory_order_relaxed);

  __sync_synchronize();

  atomic_store_explicit(&queue->bottom, job_index + 1, memory_order_relaxed);
  return 0;
}

job *work_stealing_queue_pop(work_stealing_queue *queue) {
  uint32_t bottom =
      atomic_load_explicit(&queue->bottom, memory_order_relaxed) - 1;

  atomic_store_explicit(&queue->bottom, bottom, memory_order_relaxed);

  // Make sure m_bottom is published before reading top.
  // Requires a full StoreLoad memory barrier, even on x86/64.
  __atomic_thread_fence(memory_order_seq_cst);

  uint32_t top = atomic_load_explicit(&queue->top, memory_order_relaxed);
  if (top <= bottom) {
    job *job = atomic_load_explicit(
        &queue->entries[bottom & (queue->capacity - 1)], memory_order_relaxed);
    if (top != bottom) {
      // still >0 jobs left in the queue
      return job;
    } else {

      // popping the last element in the queue
      if (!atomic_compare_exchange_strong_explicit(&queue->top, &top, top + 1,
                                                   memory_order_seq_cst,
                                                   memory_order_relaxed)) {
        // failed race against Steal()
        job = NULL;
      }
      atomic_store_explicit(&queue->bottom, top + 1, memory_order_relaxed);
      return job;
    }
  } else {
    // queue already empty

    atomic_store_explicit(&queue->bottom, top, memory_order_relaxed);
    return NULL;
  }
}

job *work_stealing_queue_steal(work_stealing_queue *queue) {
  uint32_t top = atomic_load_explicit(&queue->top, memory_order_acquire);

  // Ensure top is always read before bottom.
  // A LoadLoad memory barrier would also be necessary on platforms with a weak
  // memory model.
  __sync_synchronize();

  uint32_t bottom = atomic_load_explicit(&queue->bottom, memory_order_acquire);
  if (top < bottom) {
    job *job = atomic_load_explicit(
        &queue->entries[top & (queue->capacity - 1)], memory_order_consume);
    // CAS serves as a compiler barrier as-is.
    if (!atomic_compare_exchange_strong_explicit(&queue->top, &top, top + 1,
                                                 memory_order_seq_cst,
                                                 memory_order_relaxed)) {
      // concurrent Steal()/Pop() got this entry first.
      return NULL;
    }
    atomic_store_explicit(&queue->entries[top & (queue->capacity - 1)], NULL,
                          memory_order_relaxed);
    return job;
  } else {
    return NULL; // queue empty
  }
}