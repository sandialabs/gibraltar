/* gib_context.h: Defines the Gibraltar context structure.
 *
 * Copyright (C) University of Alabama at Birmingham and Sandia
 * National Laboratories, 2010 - 2014, written by Matthew L. Curry
 * <mlcurry@sandia.gov>
 *
 * Edited by Mathew L. Curry and Rodrigo A. Sardinas on Dec, 2014
 * <ras0054@tigermail.auburn.edu>
 *
 * Changes:
 * 1) added dynamic_fp struct to gib_context_t struct,
 * this allows for the gib_context_t struct to become
 * implementation specific (cuda, cpu, jerasure).
 */
#include "dynamic_fp.h"

#ifndef GIB_CONTEXT_H_
#define GIB_CONTEXT_H_

struct gib_context_t {
	int n, m;
	unsigned char *F;
	/* The stuff below is only used in the GPU case */
	void *acc_context;
	struct dynamic_fp * strategy;

};

typedef struct gib_context_t* gib_context;



#endif /*GIB_CONTEXT_H_*/
