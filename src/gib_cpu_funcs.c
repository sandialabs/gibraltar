/* Author:  Matthew Curry
 * email:   curryml@cis.uab.edu
 * 
 * An outer wrapper to allow easy choice between a CPU and GPU implementation
 * while bringing up the GPU stuff.
 */
#include "../inc/gibraltar.h"
#include "../inc/gib_galois.h"
#include "../inc/gib_cpu_funcs.h"
#include <stdlib.h>
#include <stdio.h>

int gib_cpu_init ( int n, int m, gib_context *c ) {
  int rc;
  if (gib_galois_init()) {
    return GIB_ERR;
  }
  
  *c = (gib_context) malloc(sizeof(struct gib_context_t));
  if (c == NULL)
    return GIB_OOM;
  (*c)->n = n;
  (*c)->m = m;
  
  (*c)->F = (unsigned char *)malloc(m*n);
  if ((*c)->F == NULL)
    return GIB_OOM;
  
  if ((rc = gib_galois_gen_F((*c)->F, m, n)))
    return rc;
  
  return 0;
}

int gib_cpu_destroy ( gib_context c ) {
  free(c->F);
  free(c);
  return 0;
}

int gib_cpu_alloc ( void **buffers, int buf_size, int *ld, gib_context c ) {
  /* In order to improve the performance of this routine, the stride can be
   * altered through the ld parameter.  The user can continue assuming the
   * buf_size is the same if he/she wants, but the routines may run slower.
   * 
   * Athough this CPU implementation is not performance-based, it operates
   * twice as fast if the stride is odd, so it is done.
   */
  if (buf_size % 2 == 0)
    buf_size += 1;
  
  if (ld != NULL)
    (*ld) = buf_size;
  
  *buffers = malloc((c->n+c->m)*buf_size);
  if (*buffers == NULL) {
    return GIB_OOM;
  }
  return 0;
}

int gib_cpu_free ( void *buffers ) {
  free(buffers);
  return 0;
}

int gib_cpu_generate ( void *buffers, int buf_size, gib_context c ) {
  return gib_generate_nc(buffers, buf_size, buf_size, c);
}

int gib_cpu_generate_nc ( void *buffers, int buf_size, int work_size,  
			  gib_context c) {
  /* This is a noncontiguous implementation, which may be added to Gibraltar
   * eventually.
   */
  unsigned char *c_buf = (unsigned char *)buffers;
  int i, b, j, tmp;
  int m = c->m;
  int n = c->n;
  for (b = 0; b < work_size; b++) {
    for (tmp = n; tmp < m + n; ++tmp) {
      c_buf[tmp*buf_size+b] = 0;
    }
    for (j = 0; j < m; ++j) {
      for (i = 0; i < n; ++i) {
	c_buf[(n+j)*buf_size+b] ^= 
	  gib_gf_table[c->F[j*n+i]][c_buf[i*buf_size+b]];
      }
    }
  }
  return 0;
}

int gib_cpu_recover ( void *buffers, int buf_size, int *buf_ids, 
		      int recover_last, gib_context c ) {
  return gib_recover_nc(buffers, buf_size, buf_size, buf_ids, recover_last, 
			c);
}

int gib_cpu_recover_nc ( void *buffers, int buf_size, int work_size, 
			 int *buf_ids, int recover_last,gib_context c ) {
  /* This is a noncontiguous implementation, which may be added to Gibraltar
   * eventually.
   */
  int i, j;
  
  unsigned char *c_buf = (unsigned char *)buffers;
  int n = c->n;
  int m = c->m;
  unsigned char A[128*128], inv[128*128], modA[128*128];
  
  for (i = n; i < n+recover_last; i++) {
    if (buf_ids[i] >= n) {
      /* Recovering a parity buffer is not a valid operation. */
      fprintf(stderr, "Attempting to recover a parity buffer, aborting.\n");
      return GIB_ERR;
    }
  }
  
  gib_galois_gen_A(A, m+n, n);
  
  /* Modify the matrix to have the failed drives reflected */
  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      modA[i*n+j] = A[buf_ids[i]*n+j];
    }
  }
  
  gib_galois_gaussian_elim(modA, inv, n, n);
  
  /* Copy row buf_ids[i] into row i */
  for (i = n; i < n+recover_last; i++)
    for (j = 0; j < n; j++)
      modA[i*n+j] = inv[buf_ids[i]*n+j];
  
  int b, tmp;
  for (b = 0; b < work_size; b++) {
    for (tmp = n; tmp < recover_last + n; ++tmp) {
      c_buf[tmp*buf_size+b] = 0;
    }
    for (j = n; j < n+recover_last; ++j) {
      for (i = 0; i < n; ++i) {
	c_buf[j*buf_size+b] ^= 
	  gib_gf_table[modA[j*n+i]][c_buf[i*buf_size+b]];
      }
    }
  }
  return 0;
}
