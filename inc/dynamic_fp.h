/* dynamic_fp.h: Defines the dynamic function pointer structure.
 *
 * Copyright (C) University of Alabama at Birmingham and Sandia
 * National Laboratories, 2010 - 2014, written by Matthew L. Curry
 * and Rodrigo A. Sardinas
 * <mlcurry@sandia.gov> <ras0054@tigermail.auburn.edu>
 *
 * Changes:
 */
#ifndef DYNAMIC_FP_
#define DYNAMIC_FP_

struct gib_context_t;

struct dynamic_fp {

int (*gib_destroy)(struct gib_context_t *c);
int (*gib_alloc)(void **buffers, int buf_size, int *ld, struct gib_context_t *c);
int (*gib_free)(void *buffers, struct gib_context_t *c);
int (*gib_generate)(void *buffers, int buf_size, struct gib_context_t *c);
int (*gib_generate_nc)(void *buffers, int buf_size, int work_size,
		    struct gib_context_t *c);
int (*gib_recover)(void *buffers, int buf_size, int *buf_ids, int recover_last,
		struct gib_context_t *c);
int (*gib_recover_nc)(void *buffers, int buf_size, int work_size, int *buf_ids,
		int recover_last, struct gib_context_t *c);

};

extern struct dynamic_fp cuda, jerasure, cpu;

#endif