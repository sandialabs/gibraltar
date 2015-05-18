/* dynamic_fp.h: Defines the dynamic function pointer structure.
 *
 * Copyright (C) University of Alabama at Birmingham and Sandia
 * National Laboratories, 2010, written by and Rodrigo A. Sardinas
 * <ras0054@tigermail.auburn.edu>, under contract to Sandia National
 * Laboratories.
 *
 * Changes:
 * Initial version, Rodrigo A. Sardinas
 */

#ifndef _GIB_DYNAMIC_FP_H_
#define _GIB_DYNAMIC_FP_H_

struct gib_context_t;

struct dynamic_fp {
	int (*gib_destroy)(struct gib_context_t *c);
	int (*gib_alloc)(void **buffers, int buf_size, int *ld,
			 struct gib_context_t *c);
	int (*gib_free)(void *buffers, struct gib_context_t *c);
	int (*gib_generate)(void *buffers, int buf_size,
			    struct gib_context_t *c);
	int (*gib_generate2)(unsigned char **buffers, unsigned int buf_size,
			    struct gib_context_t *c);
	int (*gib_generate_nc)(void *buffers, int buf_size, int work_size,
			       struct gib_context_t *c);
	int (*gib_recover)(void *buffers, int buf_size, int *buf_ids,
			   int recover_last, struct gib_context_t *c);
	int (*gib_recover2)(unsigned char **buffers, unsigned int buf_size, unsigned int *buf_ids,
			    unsigned int recover_last, struct gib_context_t *c);
	int (*gib_recover_nc)(void *buffers, int buf_size, int work_size,
			      int *buf_ids, int recover_last,
			      struct gib_context_t *c);
};

extern struct dynamic_fp cuda, jerasure, cpu;

#endif
