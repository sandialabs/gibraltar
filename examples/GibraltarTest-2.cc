// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 

#include <iomanip>
#include "GibraltarTest.h"
#include <gibraltar.h>

#ifdef LARGE_ENOUGH
#undef LARGE_ENOUGH
#endif
#define LARGE_ENOUGH 256
using namespace std;

void
GibraltarTest::run_test() {
  int n = 2, m = 3;

  int rc = gib_init_cuda(n, m, &gc);

  cout << "n = " << n << " m = " << m << endl;

  if (rc) {
    cout << "Error: " << rc << endl;
    exit(EXIT_FAILURE);
  }

  /*      cout << "step 2." << endl; */
  int size = LARGE_ENOUGH;
  unsigned char *data = 
    static_cast<unsigned char *>(malloc(size * (n + m)));

  const char *payload0 =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

  const char *payload1 =
    "Take a look at this for more detail. It refers to how Apache h";
    "andles multiple requests. Preforking, which is the default, st";

    for (int i = 0; i < size; i++) {
      data[i] = payload0[i % size];
    }

    for (int i = size; i < size + 1; i++) {
      data[i] = payload1[i % size];
    }

  gib_generate(data, size, gc);

  cout << "Data and Coding for n = 2, m = 3." << endl; 
  unsigned char ch;
  for (int i = 0; i < (m + n) * size; i++) {
    ch = data[i];
    cout << hex << uppercase << (ch < 32 ? '.' : ch);
    if ( i % 16 == 0 ) cout << endl;
    if ( i % size == 0 ) cout << endl << endl;
  }
  cout << endl << endl;

  unsigned char *backup_data = 
    static_cast<unsigned char *>(malloc(size * (n + m)));

  /*      cout << "step 4." << endl; */
  memcpy(backup_data, data, size * (n + m));

  char failed[256];
  for (unsigned int i = 0; i < n + m; i++)
    failed[i] = 0;
  for (int i = 0; i < ((m < n) ? m : n); i++) {
    failed[i] = 1;

    /* Destroy the buffer */
    memset(data + size * i, 0, size);
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

  unsigned char *dense_data = 
    static_cast<unsigned char *>(malloc(size * (n + m)));

  for (int i = 0; i < m + n; i++) {
    memcpy(dense_data + i * size,
	   data + buf_ids[i] * size, size);
  }

  int nfailed = (m < n) ? m : n;
  memset(dense_data + n * size, 0,
	 size * nfailed);

  cout << "Data and Coding for n = 2, m = 3 after destroying data chunks and making dense." << endl; 
  for (int i = 0; i < (m + n) * size; i++) {
    ch = dense_data[i];
    cout << hex << uppercase << (ch < 32 ? '.' : ch);
    if ( i % 16 == 0 ) cout << endl;
    if ( i % size == 0 ) cout << endl << endl;
  }
  cout << endl << endl;

  gib_recover(dense_data, size, buf_ids, nfailed, gc);

      for (unsigned int i = 0; i < m + n; i++) {
	if (memcmp(dense_data + i * size,
		   backup_data + buf_ids[i] * size, size)) {
	  printf("Dense test failed on buffer %i/%i.\n", i,
		 buf_ids[i]);
	  exit(1);
	}
      }

  cout << "Data and Coding for n = 2, m = 3 after recovering data chunks." << endl; 
  for (int i = 0; i < (m + n) * size; i++) {
    ch = dense_data[i];
    cout << hex << uppercase << (ch < 32 ? '.' : ch);
    if ( i % 16 == 0 ) cout << endl;
    if ( i % size == 0 ) cout << endl << endl;
  }
  cout << endl << endl;

  free(backup_data);
  gib_destroy(gc);
}

int main(int argc,char **args) {
  GibraltarTest test;
  test.run_test();
  return 0;
}
