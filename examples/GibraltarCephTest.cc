// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 

#include <iomanip>
#include "GibraltarCephTest.h"
#include <gibraltar.h>

// Ceph include headers
#include <errno.h>

#include "crush/CrushWrapper.h"
#include "include/stringify.h"
#include "global/global_init.h"
#include "common/ceph_argparse.h"
#include "global/global_context.h"

/* May use these later
#include "erasure-code/gibraltar/ErasureCodeGibraltar.h"
#include "gtest/gtest.h"
*/

#ifndef LARGE_ENOUGH
#define LARGE_ENOUGH 2048
#endif

using namespace std;

void
GibraltarCephTest::run_test() {
  int j = 0;
  int iters = 5;
  cout << "Speed test with correctness checks" << endl;
  cout << "datasize is N*bufsize, or the total size of all data buffers"
       << endl;
  cout << "                         "
       << "cuda            cuda    " << endl;
  cout << "n    m       datasize    "
       << "chk_tput        rec_tput" << endl;

  const char *payload =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

  for (unsigned int m = min_test; m <= max_test; m++) {
    for (unsigned int n = min_test; n <= max_test; n++) {
      double chk_time, dns_time;
      cout << n  << setw(4) << m << endl;

      cout << "step 1. m = " << m << " n = " << n << endl; 
      int rc = gib_init_cuda(n, m, &gc);
      if (rc) {
	cout << "Error: " << rc << endl;
	exit(EXIT_FAILURE);
      }

      cout << "step 2a." << endl;
      int size = LARGE_ENOUGH;
      char * chunks[n+m];
      std::map<int,bufferlist> data;
      cout << "step 2b." << endl;
      for(int i = 0; i < n; i++) {
	bufferptr in_ptr(buffer::create_page_aligned(LARGE_ENOUGH));
	in_ptr.zero();
	in_ptr.set_length(0);
	in_ptr.append(payload, strlen(payload));
	cout << "step 2b.i." << i << endl;
	bufferlist bl;
	data.insert(std::pair<int,bufferlist>(i,bl));
	bl.push_front(in_ptr);
	chunks[i] = bl.c_str();
	cout << "step 2b.ii." << i << endl;
      }

      cout << "step 2c." << endl;
      for(int i = n; i < m + n; i++) {
	bufferptr in_ptr(buffer::create_page_aligned(LARGE_ENOUGH));
	in_ptr.zero();
	in_ptr.set_length(0);
	bufferlist in;
	in.push_front(in_ptr);
	chunks[i] = in.c_str();
	data.insert(std::pair<int,bufferlist>(i,in));
      }

      cout << "step 3." << endl;
      time_iters(chk_time, gib_generate2(chunks, size, gc), iters);

      cout << "step 4." << endl;
      char * backup_chunks[n+m];
      std::map<int,bufferlist> backup_data;
      for (int i = 0; i < m + n; i++) {
	bufferptr in_ptr(buffer::create_page_aligned(LARGE_ENOUGH));
	in_ptr.zero();
	in_ptr.set_length(0);
	bufferlist in;
	in.push_front(in_ptr);
	data[i].copy(0, size, in);
	backup_chunks[i] = in.c_str();
	backup_data.insert(std::pair<int,bufferlist>(i,in));
      }

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
	data[i].zero(0,size);
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

      char * dense_chunks[n+m];
      std::map<int,bufferlist> dense_data;
      for (int i = 0; i < m + n; i++) {
	bufferptr in_ptr(buffer::create_page_aligned(LARGE_ENOUGH));
	in_ptr.zero();
	in_ptr.set_length(0);
	bufferlist in;
	in.push_front(in_ptr);
	data[i].copy(0, size, in);
	dense_chunks[i] = in.c_str();
	dense_data.insert(std::pair<int,bufferlist>(i,in));
      }

      int nfailed = (m < n) ? m : n;
      for (int i = n;i<nfailed;i++)
	dense_data[i].zero(0, size);
      time_iters(dns_time,
		 gib_recover2(dense_chunks, size, buf_ids, nfailed, gc),
		 iters);

      for (unsigned int i = 0; i < m + n; i++) {
	if (memcmp(dense_chunks[i],
		   backup_chunks[buf_ids[i]],size)) {
	  cout << "Dense test failed on buffer " << i << " / " <<  buf_ids[i] << endl;
	  cout << dense_chunks[i] << endl;
	  cout << backup_chunks[buf_ids[i]] << endl;
	  exit(1);
	}
      }


      cout << "step 5." << endl;

      double size_mb = size * n / 1024.0 / 1024.0;

      if(j==0) cout << setw(10) << size * n;

      cout << setprecision(10);
      cout << setw(20) << size_mb / chk_time
	   << setw(20) << size_mb / dns_time << endl;

      cout << "step 6." << endl;
      gib_destroy(gc);
    }
  }
}

int main(int argc,char **args) {
  GibraltarCephTest test;
  test.run_test();
  return 0;
}
