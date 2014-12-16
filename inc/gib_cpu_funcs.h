/* gib_cpu_funcs.h: Interal CPU-based interface for Gibraltar
 *
 * Copyright (C) University of Alabama at Birmingham and Sandia
 * National Laboratories, 2010, written by Matthew L. Curry
 * <mlcurry@sandia.gov>
 *
 * Changes:
 *
 */
#include "gibraltar.h"
#ifdef __cplusplus
extern "C" {
#endif

int gib_cpu_init(int n, int m, struct gib_context_t **c);
int gib_cpu_destroy(struct gib_context_t *c);
int gib_cpu_alloc(void **buffers, int buf_size, int *ld, struct gib_context_t *c);
int gib_cpu_free(void *buffers);
int gib_cpu_generate(void *buffers, int buf_size, struct gib_context_t *c);
int gib_cpu_generate_nc(void *buffers, int buf_size, int work_size,
			struct gib_context_t *c);
int gib_cpu_recover_sparse(void *buffers, int buf_size, char *failed_bufs,
			   struct gib_context_t *c);
int gib_cpu_recover_sparse_nc(void *buffers, int buf_size, int work_size,
			      char *failed_bufs, struct gib_context_t *c);
int gib_cpu_recover(void *buffers, int buf_size, int *buf_ids,
		    int recover_last, struct gib_context_t *c);
int gib_cpu_recover_nc(void *buffers, int buf_size, int work_size,
		       int *buf_ids, int recover_last, struct gib_context_t *c);

#ifdef __cplusplus
}
#endif

