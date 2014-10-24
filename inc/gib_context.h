/* Author:  Matthew Curry
 * Email:   curryml@cis.uab.edu
 *
 * Defines the Gibraltar context object, which contains the properties, matrix,
 * and accelerator context.  It is used for most Gibraltar calls.
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
