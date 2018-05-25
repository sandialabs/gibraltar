/* dynamic_gibraltar.c: Implementation of public interface for the
 * Gibraltar library
 *
 * Copyright (C) University of Alabama at Birmingham and Sandia
 * National Laboratories, 2010, written by and Rodrigo A. Sardinas
 * <ras0054@tigermail.auburn.edu>, under contract to Sandia National
 * Laboratories.
 *
 * Changes:
 * Initial version, Matthew L. Curry
 *
 */

#include <gibraltar.h>
#include "gib_context.h"
#include "dynamic_fp.h"

/* Functions */

int
gib_destroy(gib_context c)
{
	return c->strategy->gib_destroy(c);
}

int
gib_alloc(void **buffers, int buf_size, int *ld, gib_context c)
{
	return c->strategy->gib_alloc(buffers, buf_size, ld, c);
}

int
gib_free(void *buffers, gib_context c)
{
	return c->strategy->gib_free(buffers, c);
}

int
gib_generate(void *buffers, int buf_size, gib_context c)
{
	return c->strategy->gib_generate(buffers, buf_size, c);
}

int
gib_generate_nc(void *buffers, int buf_size, int work_size,
		    gib_context c)
{
	return c->strategy->gib_generate_nc(buffers, buf_size, work_size, c);
}

int
gib_recover(void *buffers, int buf_size, int *buf_ids, int recover_last,
		gib_context c)
{
	return c->strategy->gib_recover(buffers, buf_size, buf_ids,
					recover_last, c);
}

int
gib_recover_nc(void *buffers, int buf_size, int work_size, int *buf_ids,
		   int recover_last, gib_context c)
{
	return c->strategy->gib_recover_nc(buffers, buf_size, work_size,
					   buf_ids, recover_last, c);
}
