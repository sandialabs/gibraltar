/* gibraltar_cpu.c: Basic, low-performance CPU implementation.
 *
 * Copyright (C) University of Alabama at Birmingham and Sandia
 * National Laboratories, 2010, written by Matthew L. Curry
 * <mlcurry@sandia.gov>
 *
 * Changes:
 *
 */

#include "../inc/gibraltar.h"
#include "../inc/gib_galois.h"
#include "../inc/gib_cpu_funcs.h"
#include <stdlib.h>
#include <stdio.h>

int
gib_init(int n, int m, gib_context *c)
{
	return gib_cpu_init(n, m, c);
}

int
gib_destroy(gib_context c)
{
	return gib_cpu_destroy(c);
}

int
gib_alloc(void **buffers, int buf_size, int *ld, gib_context c)
{
	return gib_cpu_alloc(buffers, buf_size, ld, c);
}

int
gib_free(void *buffers, gib_context c)
{
	return gib_cpu_free(buffers);
}

int
gib_generate(void *buffers, int buf_size, gib_context c)
{
	return gib_cpu_generate(buffers, buf_size, c);
}

int
gib_generate_nc(void *buffers, int buf_size, int work_size,
		gib_context c)
{
	return gib_cpu_generate_nc(buffers, buf_size, work_size, c);
}

int
gib_recover(void *buffers, int buf_size, int *buf_ids, int recover_last,
	    gib_context c)
{
	return gib_cpu_recover(buffers, buf_size, buf_ids, recover_last, c);
}

int
gib_recover_nc(void *buffers, int buf_size, int work_size, int *buf_ids,
	       int recover_last, gib_context c)
{
	return gib_cpu_recover_nc(buffers, buf_size, work_size, buf_ids,
				  recover_last, c);
}
