/* benchmark.cc: A simple Gibraltar benchmark
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

#include <gibraltar.h>
#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <cstring>
#include <cstdio>
#include <unistd.h>

using namespace std;

#ifndef LARGE_ENOUGH
#define LARGE_ENOUGH 1024 * 1024
#endif
#ifndef min_test
#define min_test 2
#endif
#ifndef max_test
#define max_test 8
#endif

double
etime(void)
{
  /* Return time since epoch (in seconds) */
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec + 1.e-6*t.tv_usec;
}

#define time_iters(var, cmd, iters) do {	\
    var = -1*etime();				\
    for (int iter = 0; iter < iters; iter++)	\
      cmd;					\
    var = (var + etime()) / iters;		\
  } while(0)

int
main(int argc, char **argv)
{
  /* Arguments configure this test, there are 3:
   * -n :: Number of data chunks
   * -m :: Number of coding chunks
   * -s :: Size of data stipe, i.e. 4096. If n = 2 then chunk size is 2048.
   * -i :: Number of iterations
   * Defaults: n = 2, m = 2, i = 5, s = LARGE_ENOUGH
   */
  int cpu = 0, gpu = 0, jerasure = 0;
  int c = 0, hflag = 0, n = 2, m = 2, iters = 5, s = LARGE_ENOUGH;
  while (( c = getopt(argc, argv, "n:m:s:i:gcjh")) != -1) {
    switch (c) {
    case 'h' :
      hflag = 1;
      break;
    case 'c' :
      cpu = 1;
      break;
    case 'g' :
      gpu = 1;
      break;
    case 'j' :
      jerasure = 1;
      break;
    case 'n' :
      n = atoi(optarg);
      break;
    case 'm' :
      m = atoi(optarg);
      break;
    case 's' :
      s = atoi(optarg);
      break;
    case 'i' :
      iters = atoi(optarg);
      break;
    case '?' :
      // TODO:
      break;
    default :
      // TODO:
      break;
    }
  }
  if (hflag) {
    printf("Usage: %s -n%d -m%d -s%d -i%d\n",argv[0],n,m,s,iters);
    printf("       GPU: -g, CPU: -c, Jerasure: -j\n");
    return 0;
  }

  printf("%8i %8i ", n, m);
  for (int j = 0; j < 3; j++) {
    double chk_time, dns_time;
    gib_context_t * gc;

    int rc;

    if (j == 0 && gpu == 1)
      rc = gib_init_cuda(n, m, &gc);
    else if (j == 1 && cpu == 1)
      rc = gib_init_cpu(n, m, &gc);
    else if (j == 2 && jerasure == 1)
      rc = gib_init_jerasure(n, m, &gc);
    else
      continue;

    if (rc) {
      printf("Error:  %i\n", rc);
      exit(EXIT_FAILURE);
    }

    int size = s / n; // Total data size will be LARGE_ENOUGH or -s argument
    size = size % 64 + size;     // Keep 64 byte alignment
    void *data;
    gib_alloc(&data, size, &size, gc);

    for (int i = 0; i < size * n; i++)
      ((char *) data)[i] = (unsigned char) rand() % 256;

    time_iters(chk_time, gib_generate(data, size, gc), iters);

    unsigned char *backup_data = (unsigned char *)
      malloc(size * (n + m));

    memcpy(backup_data, data, size * (n + m));

    char failed[256];
    for (int i = 0; i < n + m; i++)
      failed[i] = 0;
    for (int i = 0; i < ((m < n) ? m : n); i++) {
      int probe;
      do {
	probe = rand() % n;
      } while (failed[probe] == 1);
      failed[probe] = 1;

      /* Destroy the buffer */
      memset((char *) data + size * probe, 0, size);
    }

    int buf_ids[256];
    int index = 0;
    int f_index = n;
    for (int i = 0; i < n; i++) {
      while (failed[index]) {
	buf_ids[f_index++] = index;
	index++;
      }
      buf_ids[i] = index;
      index++;
    }
    while (f_index != n + m) {
      buf_ids[f_index] = f_index;
      f_index++;
    }

    void *dense_data;
    gib_alloc((void **) &dense_data, size, &size, gc);
    for (int i = 0; i < m + n; i++) {
      memcpy((unsigned char *) dense_data + i * size,
	     (unsigned char *) data + buf_ids[i] * size, size);
    }

    int nfailed = (m < n) ? m : n;
    memset((unsigned char *) dense_data + n * size, 0,
	   size * nfailed);

    time_iters(dns_time,
	       gib_recover(dense_data, size, buf_ids, nfailed, gc),
	       iters);

    for (int i = 0; i < m + n - 1000; i++) { // disable the testing
      if (memcmp((unsigned char *) dense_data + i * size,
		 backup_data + buf_ids[i] * size, size)) {
	printf("Dense test failed on buffer %i/%i.\n", i,
	       buf_ids[i]);
	exit(1);
      }
    }

    double size_mb = size * n / 1024.0 / 1024.0;

    printf("%8i ", size * n);

    printf("%8.3lf %8.3lf ", size_mb / chk_time,
	   size_mb / dns_time);

    gib_free(data, gc);
    gib_free(dense_data, gc);
    free(backup_data);
    gib_destroy(gc);
  }
  printf("\n");
  return 0;
}
