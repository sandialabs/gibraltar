/* gibraltar.h: Public interface for the Gibraltar library
 *
 * Copyright (C) University of Alabama at Birmingham and Sandia
 * National Laboratories, 2010, written by Matthew L. Curry
 * <mlcurry@sandia.gov>, Rodrigo Sardinas <ras0054@tigermail.auburn.edu>
 * under contract to Sandia National Laboratories.
 *
 * Changes:
 * Initial version, Matthew L. Curry
 * Dec 16, 2014, Rodrigo Sardinas; added init functions for
 * different back-ends.
 */

#ifndef GIBRALTAR_H_
#define GIBRALTAR_H_

#if __cplusplus
extern "C" {
#endif
struct gib_context_t;

struct gib_cuda_options {
	unsigned use_mmap; /* map host buffers into GPU instead of copying.
			      Defaults to 1. */
	const char *kernel_cache_dir;
	const char *kernel_src_dir;
};

/* Passing NULL for opts creates a context with default settings. */
int gib_init_cuda(int n, int m, struct gib_cuda_options *opts,
		  struct gib_context_t **c);
int gib_init_cpu(int n, int m, struct gib_context_t **c);
int gib_init_jerasure(int n, int m, struct gib_context_t **c);

/* Common Functions */
int gib_destroy(struct gib_context_t *c);
int gib_alloc(void **buffers, int buf_size, int *ld, struct gib_context_t *c);
int gib_free(void *buffers, struct gib_context_t *c);
int gib_generate(void *buffers, int buf_size, struct gib_context_t *c);
int gib_generate_nc(void *buffers, int buf_size, int work_size,
		    struct gib_context_t *c);
int gib_recover(void *buffers, int buf_size, int *buf_ids, int recover_last,
		struct gib_context_t *c);
int gib_recover_nc(void *buffers, int buf_size, int work_size, int *buf_ids,
		   int recover_last, struct gib_context_t *c);

/* Return codes */
static const int GIB_SUC = 0; /* Success */
static const int GIB_OOM = 1; /* Out of memory */
const static int GIB_ERR = 2; /* General mysterious error */

#if __cplusplus
}
#endif

#endif

