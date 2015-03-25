/* gib_cuda_checksum.h: Base code for Erasure Code computation for CUDA compilation
 *
 * Copyright (C) University of Alabama at Birmingham and Sandia
 * National Laboratories, 2010, written by Matthew L. Curry
 * <mlcurry@sandia.gov>, Rodrigo Sardinas <ras0054@tigermail.auburn.edu>
 * under contract to Sandia National Laboratories.
 *
 * Changes:
 * Initial version, 2010, Matthew L. Curry
 */

#ifndef GIB_CUDA_CHECKSUM_H_
#define GIB_CUDA_CHECKSUM_H_

#if __cplusplus
extern "C" {
#endif

const char gib_cuda_checksum_cu_str[] =
"typedef unsigned char byte;\n"
"__device__ unsigned char gf_log_d[256];\n"
"__device__ unsigned char gf_ilog_d[256];\n"
"__constant__ byte F_d[M*N];\n"
"__constant__ byte inv_d[N*N];\n"
"typedef int fetch;\n"
"#define nthreadsPerBlock 128\n"
"#define fetchsize 512\n"
"#define SOF 4\n"
"#define nbytesPerThread SOF \n"
"#define ROUNDUPDIV(x,y) ((x + y - 1)/y)\n"
"union shmem_bytes {\n"
"  fetch f;\n"
"  byte b[SOF];\n"
"};\n"
"__shared__ byte sh_log[256];\n"
"__shared__ byte sh_ilog[256];\n"
"__device__ __inline__ void load_tables(uint3 threadIdx, const dim3 blockDim) {\n"
"  int iters = ROUNDUPDIV(256,fetchsize);\n"
"  for (int i = 0; i < iters; i++) {\n"
"    if (i*fetchsize/SOF+threadIdx.x < 256/SOF) {\n"
"      int fetchit = threadIdx.x + i*fetchsize/SOF;\n"
"      ((fetch *)sh_log)[fetchit] = *(fetch *)(&gf_log_d[fetchit*SOF]);\n"
"      ((fetch *)sh_ilog)[fetchit] = *(fetch *)(&gf_ilog_d[fetchit*SOF]);\n"
"    }\n"
"  }\n"
"}\n"
"__global__ void gib_recover_d(shmem_bytes *bufs, int buf_size,\n"
"			      int recover_last) {\n"
"  int rank = threadIdx.x + __umul24(blockIdx.x, blockDim.x);\n"
"  load_tables(threadIdx, blockDim);\n"
"  shmem_bytes out[M];\n"
"  shmem_bytes in;\n"
"  for (int i = 0; i < M; i++) \n"
"    out[i].f = 0;\n"
"  __syncthreads();\n"
"  for (int i = 0; i < N; ++i) {\n"
"    in.f = bufs[rank+buf_size/SOF*i].f;\n"
"    for (int j = 0; j < recover_last; ++j) {\n"
"      int F_tmp = sh_log[F_d[j*N+i]];\n"
"      for (int b = 0; b < SOF; ++b) {\n"
"	if (in.b[b] != 0) {\n"
"	  int sum_log = F_tmp + sh_log[(in.b)[b]];\n"
"	  if (sum_log >= 255) sum_log -= 255;\n"
"	  (out[j].b)[b] ^= sh_ilog[sum_log];\n"
"	}\n"
"      }\n"
"    }\n"
"  }\n"
"  for (int i = 0; i < recover_last; i++) \n"
"    bufs[rank+buf_size/SOF*(i+N)].f = out[i].f;\n"
"}\n"
"__global__ void gib_checksum_d(shmem_bytes *bufs, int buf_size) {\n"
"#if M == 2\n"
"#undef M\n"
"#define M 3\n"
"#define RAID6_FIX    \n"
"#endif\n"
"  int rank = threadIdx.x + __umul24(blockIdx.x, blockDim.x);\n"
"  load_tables(threadIdx, blockDim);\n"
"  shmem_bytes out[M];\n"
"  shmem_bytes in;\n"
"  for (int i = 0; i < M; i++) \n"
"    out[i].f = 0;\n"
"  __syncthreads();\n"
"  for (int i = 0; i < N; ++i) {\n"
"    in.f = bufs[rank+buf_size/SOF*i].f;\n"
"    for (int j = 0; j < M; ++j) {\n"
"      int F_tmp = sh_log[F_d[j*N+i]]; /* No load conflicts */\n"
"      for (int b = 0; b < SOF; ++b) {\n"
"	if (in.b[b] != 0) {\n"
"	  int sum_log = F_tmp + sh_log[(in.b)[b]];\n"
"	  if (sum_log >= 255) sum_log -= 255;\n"
"	  (out[j].b)[b] ^= sh_ilog[sum_log];\n"
"	}\n"
"      }\n"
"    }\n"
"  }\n"
"#ifdef RAID6_FIX\n"
"#undef M\n"
"#define M 2\n"
"#undef RAID6_FIX\n"
"#endif\n"
"  for (int i = 0; i < M; i++) \n"
"    bufs[rank+buf_size/SOF*(i+N)].f = out[i].f;\n"
"}\n";

#if __cplusplus
}
#endif

#endif
