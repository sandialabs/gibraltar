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
#define LARGE_ENOUGH 256
#endif

const unsigned GibraltarCephTest::SIMD_ALIGN = 32;

using namespace std;

void
GibraltarCephTest::run_test() {

  const char *payload0 =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

  const char *payload1 =
    "Take a look at this for more detail. It refers to how Apache h";
  "andles multiple requests. Preforking, which is the default, st";

  const char *zeros =
    "00000000000000000000000000000000000000000000000000000000000";

  const char *ones =
    "11111111111111111111111111111111111111111111111111111111111";

  unsigned int n = 2, m = 2;
  cout << n  << setw(4) << m << endl;

  //cout << "step 1. m = " << m << " n = " << n << endl; 
  int rc = gib_init_cuda(n, m, &gc);
  if (rc) {
    cout << "Error: " << rc << endl;
    exit(EXIT_FAILURE);
  }

  //cout << "step 2a." << endl;
  int size = LARGE_ENOUGH;
  char * chunks[n+m];
  std::map<int,bufferlist> data;
  //cout << "step 2b." << endl;
  for(int i = 0; i < n; i++) {
    bufferptr in_ptr(buffer::create_aligned(LARGE_ENOUGH,SIMD_ALIGN));
    bufferlist bl;
    data.insert(std::pair<int,bufferlist>(i,bl));
    data[i].push_front(in_ptr);
    char *ptr = data[i].c_str();
    //cout << "step 2b.i." << i << endl << flush;
    if (i % 2 == 0) {
      for (unsigned int j = 0; j < size; j++) {
	ptr[j] = payload0[j % strlen(payload0)];
      }
    } else {
      for (int j = 0; j < size; j++) {
	ptr[j] = payload1[j % strlen(payload1)];
      }
    }

    //cout << "step 2b.i." << " ptr length: " << in_ptr.length() << endl << flush;
    // cout << "step 2b.ii " << i << " length of data: " << data[i].length() << endl << flush;
    data[i].rebuild_aligned_size_and_memory(LARGE_ENOUGH,SIMD_ALIGN);
    chunks[i] = data[i].c_str();

    cout << "step 2b.iii." << i << endl << flush;
    cout << "Length data[" << i << "]: " << data[i].length() << endl;
    data[i].hexdump(cout);
    cout << endl << flush;

  }

  //cout << "step 2c." << endl << flush;
  for(int i = n; i < m + n; i++) {
    bufferptr in_ptr(buffer::create_aligned(LARGE_ENOUGH,SIMD_ALIGN));
    //cout << "step 2c.ii pad_size: " << pad_size << " length: " << in_ptr.length() << endl << flush;
    bufferlist bl;
    data.insert(std::pair<int,bufferlist>(i,bl));
    data[i].push_front(in_ptr);
    data[i].rebuild_aligned_size_and_memory(LARGE_ENOUGH,SIMD_ALIGN);
    chunks[i] = data[i].c_str();
    cout << "step 2c.ii [" << i << "] length of data: " << data[i].length() << endl << flush;
    data[i].hexdump(cout);
    cout << endl << flush;
  }

  cout << "step 3. Call gib_generate2" << endl;
  gib_generate2(chunks, data[0].length(), gc);

  cout << "step 4a." << endl << flush;
  char * backup_chunks[n+m];
  std::map<int,bufferlist> backup_data;
  for (int i = 0; i < m + n; i++) {
    bufferlist bl;
    bufferptr in_ptr(buffer::create_aligned(LARGE_ENOUGH,SIMD_ALIGN));
    backup_data.insert(std::pair<int,bufferlist>(i,bl));
    backup_data[i].push_front(in_ptr);
    backup_data[i].rebuild_aligned_size_and_memory(LARGE_ENOUGH,SIMD_ALIGN);
    cout << "step 4a.i " << i << " length of data: " << data[i].length() << endl << flush;
    //data[i].copy(0, size, backup_data[i]);
    memcpy(backup_data[i].c_str(), data[i].c_str(), data[i].length());
    backup_chunks[i] = backup_data[i].c_str();
    cout << "Length backup_data[" << i << "]: " << backup_data[i].length() << endl;
    backup_data[i].hexdump(cout);
    cout << endl << flush;
  }

  cout << "step 4b." << endl << flush;
  char failed[256];
  for (unsigned int i = 0; i < n + m; i++)
    failed[i] = 0;
  for (unsigned int i = 0; i < ((m < n) ? m : n); i++) {
    failed[i] = 1;
    memset(data[i].c_str(),0,data[i].length());
  }

  cout << "step 4c. Compute buf_ids[]" << endl << flush;
  unsigned int buf_ids[256];
  unsigned int index = 0;
  unsigned int f_index = n;
  for (unsigned int i = 0; i < n; i++) {
    while (failed[index]) {
      buf_ids[f_index++] = index;
      index++;
    }
    buf_ids[i] = index;
    index++;
  }
  unsigned int nfailed = f_index - n; // Number of failed chunks.
  while (f_index != n + m) {
    buf_ids[f_index] = f_index;
    f_index++;
  }

  for(unsigned int i = 0; i < m + n; i++) {
    cout << i << "   ";
  }
  cout << endl;
  for(unsigned int i = 0; i < m + n; i++) {
    cout << buf_ids[i] << "   ";
  }
  cout << endl;

  char * dense_chunks[n+m];
  std::map<int,bufferlist> dense_data;
  for (int i = 0; i < m + n; i++) {
    bufferptr in_ptr(buffer::create_aligned(LARGE_ENOUGH,SIMD_ALIGN));
    bufferlist bl;
    dense_data.insert(std::pair<int,bufferlist>(i,bl));
    dense_data[i].push_front(in_ptr);
    dense_data[i].rebuild_aligned_size_and_memory(LARGE_ENOUGH,SIMD_ALIGN);
  }
  cout << "step 4c." << endl << flush;
  for (int i = 0; i < m + n; i++) {
    memcpy(dense_data[i].c_str(), data[buf_ids[i]].c_str(), data[i].length());
  }

  cout << "step 4d." << endl << flush;
  cout << "nfailed = " << nfailed << endl;
  cout << "size = " << size << endl;

  for (int i = 0; i < m + n; i++) {
    dense_chunks[i] = dense_data[i].c_str();
    cout << "Length dense_data[" << i << "]: " << dense_data[i].length() << endl;
    dense_data[i].hexdump(cout);
    cout << endl << flush;
  }

  cout << "step 4d. Calling gib_recover2." << endl << flush;
  gib_recover2(dense_chunks, dense_data[0].length(), buf_ids, nfailed, gc);

  cout << "step 4e." << endl << flush;
  for (unsigned int i = 0; i < m + n; i++) {
    //dense_data[i].rebuild_aligned_size_and_memory(LARGE_ENOUGH,SIMD_ALIGN);
    dense_chunks[i] = dense_data[i].c_str();
    if (memcmp(dense_chunks[i],
	       backup_chunks[buf_ids[i]],size)) {
      cout << "Dense test failed on buffer " << i << " / " <<  buf_ids[i] << endl;
      cout << "Length dense_data[" << i << "]: " << dense_data[i].length() << endl;
      dense_data[i].hexdump(cout);
      cout << endl << flush;
      cout << "Length backup_data[" << buf_ids[i] << "]: " << backup_data[i].length() << endl;
      backup_data[buf_ids[i]].hexdump(cout);
      cout << endl << flush;
      //exit(1);
    }
  }

  for (int i = 0; i < m + n; i++) {
    cout << "Length dense_data[" << i << "]: " << dense_data[i].length() << endl;
    dense_data[i].hexdump(cout);
    cout << endl << flush;

  }

  //cout << "step 6. Finished" << endl;
  gib_destroy(gc);
}

int main(int argc,char **args) {
  GibraltarCephTest test;
  test.run_test();
  return 0;
}
