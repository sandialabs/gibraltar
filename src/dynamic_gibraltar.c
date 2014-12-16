/* dynamic_gibraltar.c: Implementation of public dynamic
 * interface for the Gibraltar library
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
 * 2) depending on the gib_context_t being used, calls the appropriate
 * function
 *
 */

#include "../inc/dynamic_gibraltar.h"
#include "../inc/gib_context.h"
#include "../inc/dynamic_fp.h"

/* Functions */

int gib_destroy(gib_context c)
{
	return c->strategy->gib_destroy(c);
}

int gib_alloc(void **buffers, int buf_size, int *ld, gib_context c)
{
	return c->strategy->gib_alloc(buffers, buf_size, ld, c);
}

int gib_free(void *buffers, gib_context c)
{
	return c->strategy->gib_free(buffers, c);
}

int gib_generate(void *buffers, int buf_size, gib_context c)
{
	return c->strategy->gib_generate(buffers, buf_size, c);
}

int gib_generate_nc(void *buffers, int buf_size, int work_size,
		    gib_context c)
{
	return c->strategy->gib_generate_nc(buffers, buf_size, work_size, c);
}

int gib_recover(void *buffers, int buf_size, int *buf_ids, int recover_last,
		gib_context c)
{
	return c->strategy->gib_recover(buffers, buf_size, buf_ids, recover_last, c);
}

int gib_recover_nc(void *buffers, int buf_size, int work_size, int *buf_ids,
		   int recover_last, gib_context c)
{
	return c->strategy->gib_recover_nc(buffers, buf_size, work_size, buf_ids, recover_last, c);
}
