/* gib_context.h: Defines the Gibraltar context structure.
 *
 * Copyright (C) University of Alabama at Birmingham and Sandia
 * National Laboratories, 2010, written by Matthew L. Curry
 * <mlcurry@sandia.gov>
 *
 * Changes:
 *
 */
#ifndef GIB_CONTEXT_H_
#define GIB_CONTEXT_H_

struct gib_context_t {
	int n, m;
	unsigned char *F;
	/* The stuff below is only used in the GPU case */
	void *acc_context;
};

typedef struct gib_context_t* gib_context;

#endif /*GIB_CONTEXT_H_*/
