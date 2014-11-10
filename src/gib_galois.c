/* Author:  Matthew L. Curry
 * Email:   mlcurry@sandia.gov
 *
 * A CPU implementation of the Galois arithmetic operations needed for
 * both the low-performance CPU version of Gibraltar and initializing
 * the GPU version.
 */

#include "../inc/gib_galois.h"
#include "../inc/gibraltar.h" /* For error codes */
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

unsigned char gib_gf_log[256];
unsigned char gib_gf_ilog[256];
unsigned char gib_gf_table[256][256];

unsigned char
gib_galois_mul(unsigned char a, unsigned char b)
{
	int sum_log;

	if (a == 0 || b == 0)
		return 0;
	sum_log = gib_gf_log[a] + gib_gf_log[b];
	if (sum_log >= 255)
		sum_log -= 255;
	return gib_gf_ilog[sum_log];
}

unsigned char
gib_galois_div(unsigned char a, unsigned char b)
{
	int diff_log;

	if (a == 0)
		return 0;
	if (b == 0)
		return -1;
	diff_log = gib_gf_log[a] - gib_gf_log[b];
	if (diff_log < 0)
		diff_log += 255;
	return gib_gf_ilog[diff_log];
}

static int
gib_galois_init_unsafe(void)
{
	int i, j, b, log;
	/* This polynomial (and its use) was given as an example in
	 * James Plank's tutorial on Reed-Solomon coding for RAID.
	 */
	int prim_poly = 0435;

	memset(gib_gf_ilog, 0, 256);
	memset(gib_gf_log, 0, 256);

	b = 1;
	for (log = 0; log < 255; log++) {
		gib_gf_log[b] = (unsigned char) log;
		gib_gf_ilog[log] = (unsigned char) b;
		b = b << 1;
		if (b & 256)
			b = b ^ prim_poly;
	}

	for (i = 0; i < 256; i++) {
		for (j = 0; j < 256; j++) {
			gib_gf_table[i][j] = gib_galois_mul(i,j);
		}
	}

	return 0;
}

struct gib_galois_state {
	int rcount;
	pthread_mutex_t m;
} _gib_state = {
	.rcount = 0,
	.m = PTHREAD_MUTEX_INITIALIZER,
};

int
gib_galois_init(void)
{
	int rc = 0;
	if (pthread_mutex_lock(&_gib_state.m))
		abort();
	if (_gib_state.rcount == 0) {
		rc = gib_galois_init_unsafe();
	}
	if (rc == 0)
		_gib_state.rcount++;
	if (pthread_mutex_unlock(&_gib_state.m))
		abort();
	return rc;
}

int
gib_galois_gen_F(unsigned char *mat, int rows, int cols)
{
	/* F forms the lower portion (m x n) of A */
	int i, j, rc;
	unsigned char *tmpA = NULL;
	tmpA = (unsigned char *)malloc((rows+cols)*(cols));
	if (tmpA == NULL)
		return GIB_OOM;
	rc = gib_galois_gen_A(tmpA, rows+cols, cols);
	if (rc == 0) {
		for (i = cols; i < rows+cols; i++)
			for (j = 0; j < cols; j++)
				mat[(i-cols)*cols+j] = tmpA[i*cols+j];
	}

	free(tmpA);
	return 0;
}

int
gib_galois_gen_A(unsigned char *mat, int rows, int cols)
{
	int i, j, p;
	int x, y, z;
	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			mat[i*cols+j] = 1;
			for (p = 0; p < j; p++) {
				z = i * cols + j;
				x = mat[i * cols + j];
				y = i;
				mat[z] = gib_galois_mul(x, y);
			}
		}
	}

	gib_galois_gaussian_elim(mat, NULL, rows, cols);
	return 0;
}

int
gib_galois_gaussian_elim(unsigned char *mat, unsigned char *inv, int rows,
			 int cols)
{
	int i, j, e;
	int x, y, z;

	/* If the caller wants an inverse, inv will be not null. */
	if (inv != NULL && rows != cols) {
		/* The system is overqualified, and needs to be
		 * reduced in order to compute the inverse requested.
		 */
		return GIB_ERR;
	}

	if (inv != NULL) {
		/* Initialize to identity */
		for (i = 0; i < rows; i++) {
			for (j = 0; j < cols; j++) {
				inv[i * cols + j] = (i == j ? 1 : 0);
			}
		}
	}

	for (i = 0; i < cols; i++) {
		/* Make sure A[i][i] is nonzero by swapping */
		if (mat[i*cols+i] == 0) {
			for (j = i + 1; mat[i*cols+j] == 0 && j < cols; j++) {
				;
			}
			for (e = 0; e < rows; e++) {
				int tmp = mat[e*cols+i];
				mat[e*cols+i] = mat[e*cols+j];
				mat[e*cols+j] = tmp;
				if (inv != NULL) {
					tmp = inv[e*cols+i];
					inv[e*cols+i] = inv[e*cols+j];
					inv[e*cols+j] = tmp;
				}
			}
		}

		int inverse = gib_galois_div(1, mat[i*cols+i]);
		/* Make mat[i,i] == 1 by dividing down the column by
		 * mat[i,i] */
		for (e = 0; e < rows; e++) {
			z = e * cols + i;
			x = inverse;
			y = e * cols + i;
			mat[z] = gib_galois_mul(x, mat[y]);
			if (inv != NULL) {
				inv[z] = gib_galois_mul(x, inv[y]);
			}
		}

		/* Subtract a multiple of this column from all columns so that all
		 * mat[i,j]==0 where i != j
		 */
		for (j = 0; j < cols; j++) {
			x = mat[i * cols + j];
			if (j == i)
				continue;
			for (e = 0; e < rows; e++) {
				z = e * cols + j;
				y = e * cols + i;
				mat[z] ^= gib_galois_mul(x, mat[y]);
				if (inv != NULL) {
					inv[z] ^= gib_galois_mul(x, inv[y]);
				}
			}
		}
	}
	return 0;
}
