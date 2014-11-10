/* gibraltar.h: Public interface for the Gibraltar library
 *
 * Copyright (C) University of Alabama at Birmingham and Sandia
 * National Laboratories, 2010, written by Matthew L. Curry
 * <mlcurry@sandia.gov>
 *
 * Changes:
 *
 */
#include "gib_context.h"

#ifndef GIBRALTAR_H_
#define GIBRALTAR_H_

#if __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Functions */
int gib_init(int n, int m, gib_context *c);
int gib_destroy(gib_context c);
int gib_alloc(void **buffers, int buf_size, int *ld, gib_context c);
int gib_free(void *buffers, gib_context c);
int gib_generate(void *buffers, int buf_size, gib_context c);
int gib_generate_nc(void *buffers, int buf_size, int work_size,
		    gib_context c);
int gib_recover(void *buffers, int buf_size, int *buf_ids, int recover_last,
		gib_context c);
int gib_recover_nc(void *buffers, int buf_size, int work_size, int *buf_ids,
		   int recover_last, gib_context c);

/* Return codes */
static const int GIB_SUC = 0; /* Success */
static const int GIB_OOM = 1; /* Out of memory */
const static int GIB_ERR = 2; /* General mysterious error */

#if __cplusplus
}
#endif /* __cplusplus */

#endif /*GIBRALTAR_H_*/
