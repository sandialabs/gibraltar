/* sweeping_test.cc: A parameter-sweeping test for Gibraltar
 *
 * Copyright (C) University of Alabama at Birmingham and Sandia
 * National Laboratories, 2010, written by Matthew L. Curry
 * <mlcurry@sandia.gov>, Rodrigo Sardinas <ras0054@tigermail.auburn.edu>
 * under contract to Sandia National Laboratories.
 *
 * Changes:
 * Initial version, Matthew L. Curry
 * Dec 16, 2014, Rodrigo Sardinas; revised to use new dynamic api.
 */

/* This is a rather ridiculous example of a sweeping test.  At the parameters
 * given, it will not finish before you, the user, get bored and want to use
 * your computer again.
 *
 * That said, for every n+m configuration, it will choose every possible
 * combination of m buffers to fail with a verbose, suboptimal permutation
 * picker, call the buffer failed and recover it.  A valuable part of this
 * example that is not part of the other is its shuffling and unshuffling of
 * memory contents.  At the end of the recovery process, it is directly
 * compared to the original data buffers.  When timed, this demonstrates that
 * the memory movement is not a performance bottleneck when done properly.
 */
#include <gibraltar.h>
#include "../inc/gib_context.h"
#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <cstring>
#include <cstdio>
using namespace std;

int max_dim = 8;  /* How big can n and m be? */
int buf_size = 1024*1024/4; /* Number of integers, so scale by sizeof(int) */

int
test_config(gib_context gc, int *fail_config, int *buf,
	    const int *backup_buf)
{
	for (int i = 0; i < gc->n+gc->m; i++)
		printf("%s", (fail_config[i])?"X":".");
	printf("\n");
	/* There are n entries in fail_config, with a 1 for each
	   buffer that should be destroyed, recovered, and tested.
	*/

	int good_buffers[256]; /* n of these are needed. */
	int bad_buffers[256]; /* Up to m of these can be used */
	int ngood = 0;
	int nbad = 0;
	for (int i = 0; i < gc->n+gc->m; i++) {
		if (fail_config[i] == 0)
			good_buffers[ngood++] = i;
		else if (i < gc->n) {
			bad_buffers[nbad++] = i;
			/* destroy the buffer contents */
			memset(buf + i*buf_size, 0, buf_size);
		}
	}
	if (ngood < gc->n) {
		printf("There are not enough good buffers.\n");
		exit(1);
	}

	/* Reshuffle to prevent extraneous memory copies */
	for (int i = 0; i < ngood; i++) {
		if (good_buffers[i] != i && good_buffers[i] < gc->n) {
			int j = i+1;
			while(good_buffers[j] < gc->n)
				j++;
			int tmp = good_buffers[j];
			memmove(good_buffers+i+1, good_buffers+i,
				sizeof(int)*(j-i));
			good_buffers[i] = tmp;
		}
	}
	/* Sanity check */
	for (int i = 0; i < gc->n; i++) {
		if (good_buffers[i] != i && good_buffers[i] < gc->n) {
			printf("Didn't work...\n");
			exit(1);
		}
	}

	for (int i = 0; i < gc->n; i++) {
		if (good_buffers[i] != i) {
			memcpy(buf + buf_size * i,
			       buf + buf_size * good_buffers[i],
			       buf_size * sizeof(int));
		}
	}

	int buf_ids[256];
	memcpy(buf_ids, good_buffers, gc->n*sizeof(int));
	memcpy(buf_ids+gc->n, bad_buffers, nbad*sizeof(int));
	gib_recover(buf, buf_size*sizeof(int), buf_ids, nbad, gc);

	void *tmp_buf = malloc(sizeof(int)*buf_size);
	for (int i = 0; i < gc->n; i++) {
		if (buf_ids[i] != i) {
			int j;
			for (j = i+1; buf_ids[j] != i; j++)
				;
			memcpy(buf + buf_size * i, buf + buf_size * j,
			       buf_size * sizeof(int));
			buf_ids[i] = i;
		}
	}

	free(tmp_buf);

	return 0;
}

int
choose(int n, int m)
{
	static int *choose_dyn = NULL;
	if (choose_dyn == NULL) {
		choose_dyn =
			(int *)malloc((max_dim+1)*(2*max_dim+1)*sizeof(int));
		for (int i = 0; i < (max_dim+1)*(2*max_dim+1); i++)
			choose_dyn[i] = -1;
	}
	int answer;
	if (choose_dyn[n*max_dim+m] != -1)
		return choose_dyn[n*max_dim+m];

	if (n == m) answer = 1;
	else if (m == 0) answer = 1;
	else answer = choose(n-1, m-1) + choose(n-1, m);

	//if (n < (max_dim+1) && m < (max_dim+1))
	choose_dyn[n*max_dim+m] = answer;
	return answer;
}

void
choose_them(int n, int m, int *chosen, int counter)
{
	if (n == 0) return;
	if (n == m) {
		for (int i = 0; i < n; i++)
			chosen[i] = 1;
		return;
	}

	/* The first choose(n-1,m) don't have the first element taken */
	if (counter < choose(n-1,m)) {
		*chosen = 0;
		choose_them(n-1, m, chosen+1, counter);
	} else {
		/* The rest do. */
		*chosen = 1;
		choose_them(n-1, m-1, chosen+1, counter-choose(n-1, m));
	}
}

/* Sweeping test */
int
inc_fail(int *fail_config, gib_context gc)
{
	static int m = 0;
	static int n = 0;
	static int counter = 0;
	if (gc->n != n || gc->m != m) {
		counter = 0;
		n = gc->n;
		m = gc->m;
		for (int i = 0; i < n+m; i++)
			fail_config[i] = 0;
	}
	choose_them(n+m, m, fail_config, counter);
	counter++;
	if (counter > choose(n+m, m)) {
		return 0;
	}
	return 1;
}

int
main(int argc, char **argv)
{
	int *buf;
	int size_sc; /* scratch */
	int *backup_buf = (int *)malloc(max_dim*buf_size*sizeof(int));
	/* backup_buf just contains data */
	for (int i = 0; i < max_dim*buf_size; i++)
		backup_buf[i] = rand();

	for(int j = 0; j < 3; j++){
		for (int m = 2; m <= max_dim; m++) {
			for (int n = 2; n <= max_dim; n++) {
				fprintf(stderr, "n = %i, m = %i\n", n, m);

				gib_context_t * gc;
				int rc;

				if (j == 0) {
					printf("CUDA\n");
					rc = gib_init_cuda(n, m, NULL, &gc);
				} else if (j == 1) {
					printf("CPU\n");
					rc = gib_init_cpu(n, m, &gc);
				} else if (j == 2) {
					printf("Jerasure\n");
					rc = gib_init_jerasure(n, m, &gc);
				}

				rc = gib_alloc((void **)(&buf),
					       buf_size * sizeof(int),
					       &size_sc, gc);
				if (rc) {
					printf("Error:  %i\n", rc);
					exit(EXIT_FAILURE);
				}

				memcpy(backup_buf, buf,
				       n * buf_size * sizeof(int));
				(void)gib_generate(buf, buf_size*sizeof(int),
						   gc);

				if (memcmp(buf, backup_buf,
					   n * buf_size * sizeof(int))) {
					printf("Generation failed.\n");
					exit(1);
				}

				int *fail_config =
					(int *)malloc(sizeof(int) * (n + m));
				for (int i = 0; i < n+m; i++)
					fail_config[i] = 0;

				while(inc_fail(fail_config, gc)) {
					test_config(gc, fail_config, buf,
						    backup_buf);
					if (memcmp(buf, backup_buf,
						   n * buf_size *
						   sizeof(int))) {
						printf("Recovery failed.\n");
						exit(1);
					}
					memcpy(backup_buf, buf,
					       n * buf_size * sizeof(int));
					gib_generate(buf,
						     buf_size * sizeof(int),
						     gc);
				}

				gib_free(buf, gc);
				gib_destroy(gc);
			}
		}
	}
	return 0;
}
