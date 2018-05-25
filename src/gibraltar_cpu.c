/* gibraltar_cpu.c: Basic, low-performance CPU implementation.
 *
 * Copyright (C) University of Alabama at Birmingham and Sandia
 * National Laboratories, 2010, written by Matthew L. Curry
 * <mlcurry@sandia.gov>, Rodrigo Sardinas <ras0054@tigermail.auburn.edu>
 * under contract to Sandia National Laboratories.
 *
 * Changes:
 * Initial version, Matthew L. Curry
 * Dec 16, 2014, Rodrigo Sardinas; revised to enable dynamic use.
 *
 */

#include <gibraltar.h>
#include <stdlib.h>
#include <stdio.h>

#include "gib_context.h"
#include "gib_galois.h"
#include "gib_cpu_funcs.h"


int
gib_init_cpu(int n, int m, gib_context *c)
{
	int rc = gib_cpu_init(n,m,c);
	if (rc == GIB_SUC)
		(*c)->strategy = &cpu;
	return rc;
}

static int
_gib_destroy(gib_context c)
{
	return gib_cpu_destroy(c);
}

static int
_gib_alloc(void **buffers, int buf_size, int *ld, gib_context c)
{
	return gib_cpu_alloc(buffers, buf_size, ld, c);
}

static int
_gib_free(void *buffers, gib_context c)
{
	return gib_cpu_free(buffers);
}

static int
_gib_generate(void *buffers, int buf_size, gib_context c)
{
	return gib_cpu_generate(buffers, buf_size, c);
}

static int
_gib_generate_nc(void *buffers, int buf_size, int work_size,
		gib_context c)
{
	return gib_cpu_generate_nc(buffers, buf_size, work_size, c);
}

static int
_gib_recover(void *buffers, int buf_size, int *buf_ids, int recover_last,
	    gib_context c)
{
	return gib_cpu_recover(buffers, buf_size, buf_ids, recover_last, c);
}

static int
_gib_recover_nc(void *buffers, int buf_size, int work_size, int *buf_ids,
	       int recover_last, gib_context c)
{
	return gib_cpu_recover_nc(buffers, buf_size, work_size, buf_ids,
				  recover_last, c);
}

struct dynamic_fp cpu = {
		.gib_alloc = &_gib_alloc,
		.gib_destroy = &_gib_destroy,
		.gib_free = &_gib_free,
		.gib_generate = &_gib_generate,
		.gib_generate_nc = &_gib_generate_nc,
		.gib_recover = &_gib_recover,
		.gib_recover_nc = &_gib_recover_nc,
};

