// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 

#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <cstring>
#include <cstdio>

#if __cplusplus
extern "C" {
#endif

#include <gibraltar.h>

#ifndef min_test
#define min_test 2
#endif
#ifndef max_test
#define max_test 16
#endif

#define time_iters(var, cmd, iters) do {                                \
  var = -1*etime();                                       \
  for (int iter = 0; iter < iters; iter++)                \
    cmd;                                            \
  var = (var + etime()) / iters;                          \
  } while(0)

double
etime(void)
{
	/* Return time since epoch (in seconds) */
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec + 1.e-6*t.tv_usec;
}

class GibraltarCephTest {
 public:
  static const unsigned SIMD_ALIGN;
  gib_context_t * gc;

 GibraltarCephTest() :
  gc(0)
  {}

  virtual ~GibraltarCephTest() {}

  virtual void run_test();
};

#if __cplusplus
}
#endif
