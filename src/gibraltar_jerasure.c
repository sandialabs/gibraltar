/* Author:  Matthew L. Curry
 * Email:   mlcurry@sandia.gov
 *
 * This file implements an adapter from the Gibraltar API to the Jerasure API
 * in order to allow for non-GPU use and performance comparisons between the
 * GPU and CPU implementations.
 */

#include "../inc/gibraltar.h"
#include "../lib/Jerasure-1.2/jerasure.h"
#include "../lib/Jerasure-1.2/reed_sol.h"

int
gib_init(int n, int m, gib_context *c)
{
	int *intF;
	*c = (gib_context) malloc(sizeof(struct gib_context_t));
	if (c == NULL)
		return GIB_OOM;
	(*c)->n = n;
	(*c)->m = m;
	/* Jerasure uses an integer matrix, while Gibraltar uses an
	 * unsigned char matrix.  Pick the lesser of two evils, and
	 * just put it where it doesn't belong.
	 */
	intF = reed_sol_vandermonde_coding_matrix(n, m, 8);
	(*c)->F = (unsigned char *)intF;
	if (intF == NULL) {
		free(*c);
		return GIB_OOM;
	}
	return 0;
}

int
gib_destroy(gib_context c)
{
	free(c->F);
	free(c);

	return 0;
}

int
gib_alloc(void **buffers, int buf_size, int *ld, gib_context c)
{
	*ld = buf_size;
	*buffers = malloc((c->n+c->m)*buf_size);
	if (*buffers == NULL)
		return GIB_OOM;
	return 0;
}

int
gib_free(void *buffers, gib_context c)
{
	free(buffers);
	return 0;
}

int
gib_generate(void *buffers, int buf_size, gib_context c)
{
	char *data[256];
	char *coding[256];
	int i;

	for (i = 0; i < (c->n); i++) {
		data[i] = ((char *)buffers) + i*buf_size;
	}
	for (i = 0; i < (c->m); i++) {
		coding[i] = ((char *)buffers) + (i+(c->n))*buf_size;
	}
	jerasure_matrix_encode(c->n, c->m, 8, (int *)(c->F), data, coding,
			       buf_size);

	return 0;
}

int
gib_recover(void *buffers, int buf_size, int *buf_ids, int recover_last,
	    gib_context c)
{
	/* Gibraltar does not want to necessarily recover ALL of the
	 * lost buffers, as sometimes this is not necessary or
	 * convenient.  However, Jerasure does want to recover all
	 * buffers, and will reference any intact buffers that it
	 * pleases.  The bulk of the logic below is to address this
	 * difference.
	 */
	char *data[256];
	char *coding[256];
	int erasures[256];
	int missing[256];
	int i, counter, offset;

	for (i = 0; i < c->n+c->m; i++)
		missing[i] = 1;

	for (i = 0; i < c->n+recover_last; i++) {
		if (buf_ids[i] < c->n) {
			data[buf_ids[i]] = (char *)buffers + i*buf_size;
			missing[buf_ids[i]] = 0;
		} else {
			coding[buf_ids[i] - c->n] = (char *)buffers +
				i * buf_size;
			missing[buf_ids[i]] = 0;
		}
	}

	counter = 0;
	for (i = c->n; i < c->n+recover_last; i++) {
		erasures[counter] = buf_ids[i];
		counter++;
	}

	offset = c->n+recover_last;
	for (i = 0; i < c->n+c->m; i++) {
		if (missing[i]) {
			erasures[counter] = i;
			counter++;
			if (i < c->n)
				data[i] = (char *)buffers + offset*buf_size;
			else
				coding[i - c->n] = (char *)buffers +
					offset * buf_size;
			offset++;
		}
	}
	erasures[counter] = -1;
	jerasure_matrix_decode(c->n, c->m, 8, (int *)(c->F), 0, erasures,
			       data, coding, buf_size);
	return 0;
}
