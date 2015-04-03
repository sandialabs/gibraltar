// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 

#include <iomanip>
#include "GibraltarTest.h"
#include <gibraltar.h>

using namespace std;

void
GibraltarTest::run_test() {
  int j = 0;
  int iters = 5;
  cout << "Speed test with correctness checks" << endl;
  cout << "datasize is N*bufsize, or the total size of all data buffers"
       << endl;
  cout << "                         "
       << "cuda     cuda    cpu      cpu      jerasure jerasure" << endl;

  for (unsigned int m = min_test; m <= max_test; m++) {
    for (unsigned int n = min_test; n <= max_test; n++) {
      double chk_time, dns_time;
      /*      cout << "step 1. m = " << m << " n = " << n << endl; */
      int rc = gib_init_cuda(n, m, &gc);

      cout << n  << setw(4) << m;

      if (rc) {
	cout << "Error: " << rc << endl;
	exit(EXIT_FAILURE);
      }

      /*      cout << "step 2." << endl; */
      int size = 1024 * 1024;
      void *data;
      gib_alloc(&data, size, &size, gc);

      for (unsigned int i = 0; i < size * n; i++)
	static_cast<char *>(data)[i] = 
	  static_cast<unsigned char>(rand() % 256);

      /*      cout << "step 3." << endl; */
      time_iters(chk_time, gib_generate(data, size, gc), iters);

      unsigned char *backup_data = 
	static_cast<unsigned char *>(malloc(size * (n + m)));

      /*      cout << "step 4." << endl; */
      memcpy(backup_data, data, size * (n + m));

      char failed[256];
      for (unsigned int i = 0; i < n + m; i++)
	failed[i] = 0;
      for (int i = 0; i < ((m < n) ? m : n); i++) {
	int probe;
	do {
	  probe = rand() % n;
	} while (failed[probe] == 1);
	failed[probe] = 1;


	/* Destroy the buffer */
	memset(static_cast<char *>(data) + size * probe, 0, size);
      }

      int buf_ids[256];
      int index = 0;
      int f_index = n;
      for (unsigned int i = 0; i < n; i++) {
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
      gib_alloc(&dense_data, size, &size, gc);
      for (int i = 0; i < m + n; i++) {
	memcpy(static_cast<unsigned char *>(dense_data) + i * size,
	       static_cast<unsigned char *>(data) + buf_ids[i] * size, size);
      }

      int nfailed = (m < n) ? m : n;
      memset(static_cast<unsigned char *>(dense_data) + n * size, 0,
	     size * nfailed);
      time_iters(dns_time,
		 gib_recover(dense_data, size, buf_ids, nfailed, gc),
		 iters);

      for (unsigned int i = 0; i < m + n; i++) {
	if (memcmp(static_cast<unsigned char *>(dense_data) + i * size,
		   backup_data + buf_ids[i] * size, size)) {
	  printf("Dense test failed on buffer %i/%i.\n", i,
		 buf_ids[i]);
	  exit(1);
	}
      }


      /*      cout << "step 5." << endl; */

      double size_mb = size * n / 1024.0 / 1024.0;

      if(j==0) cout << setw(10) << size * n;

      cout << setprecision(10);
      cout << setw(20) << size_mb / chk_time
	   << setw(20) << size_mb / dns_time << endl;

      /*      cout << "step 6." << endl; */
      gib_free(data, gc);
      gib_free(dense_data, gc);
      free(backup_data);
      gib_destroy(gc);
    }
  }
}

int main(int argc,char **args) {
  GibraltarTest test;
  test.run_test();
  return 0;
}
