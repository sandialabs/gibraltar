/* dynamic_gibraltar.h: Public dynamic interface for the Gibraltar library
 *
 * Copyright (C) University of Alabama at Birmingham and Sandia
 * National Laboratories, 2010-2014, written by Matthew L. Curry
 * <mlcurry@sandia.gov>
 *
 * Edited by Mathew L. Curry and Rodrigo A. Sardinas on Dec, 2014
 * <ras0054@tigermail.auburn.edu>
 *
 *
 * Changes:
 * 1) previously only allowed for one implementation to be
 * active at a time. Running a different implementation required
 * recompilation. Currently allows for running any implementation
 * (cuda, cpu, jerasure) dynamically without the need to compile for
 * a specific one.
 *
 */

#ifndef DYNAMIC_GIBRALTAR_H_
#define DYNAMIC_GIBRALTAR_H_

#if __cplusplus
extern "C" {
#endif
struct gib_context_t;

//GPU init
int gib_init_cuda(int n, int m, struct gib_context_t **c);

//CPU init
int gib_init_cpu(int n, int m, struct gib_context_t **c);

//Jerasure init
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

