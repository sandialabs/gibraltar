/* gib_cuda_checksum.cu: CUDA kernels for Reed-Solomon coding.
 *
 * Copyright (C) University of Alabama at Birmingham and Sandia
 * National Laboratories, 2010, written by Matthew L. Curry
 * <mlcurry@sandia.gov>
 *
 * Changes:
 * - Converted to GGEMM implementation.
 */

typedef unsigned char byte;
__device__ unsigned char gf_log_d[256];
__device__ unsigned char gf_ilog_d[256];
__constant__ byte F_d[M*N];
__constant__ byte inv_d[N*N];

/* The "fetch" datatype is the unit for performing data copies between areas of
 * memory on the GPU.  While today's wisdom says that 32-bit types are optimal
 * for this, I want to easily experiment with the others.
 */
typedef int fetch;
#define nthreadsPerBlock 128

/* These quantities must be hand-recalculated, as the compiler doesn't seem to
 * always do such things at compile time.
 */
/* fetchsize = nthreadsPerBlock * sizeof(fetch) */
#define fetchsize 512
/* size of fetch, i.e. sizeof(fetch)*/
#define SOF 4
#define nbytesPerThread SOF

#define ROUNDUPDIV(x,y) ((x + y - 1) / y)

/* We're pulling buffers from main memory based on the fetch type, but want
 * to index into it at the byte level.
 */
union shmem_bytes {
  fetch f;
  byte b[SOF];
};

/* Shared memory copies of pertinent data */
__shared__ byte sh_log[256];
__shared__ byte sh_ilog[256];

__device__ __inline__ void load_tables(uint3 threadIdx, const dim3 blockDim) {
  /* Fully arbitrary routine for any blocksize and fetch size to load
   * the log and ilog tables into shared memory.
   */
	int iters = ROUNDUPDIV(256, fetchsize);
	for (int i = 0; i < iters; i++) {
		if (i * fetchsize / SOF + threadIdx.x < 256 / SOF) {
			int fetchit = threadIdx.x + i*fetchsize/SOF;
			((fetch *)sh_log)[fetchit] =
				*(fetch *)(&gf_log_d[fetchit*SOF]);
			((fetch *)sh_ilog)[fetchit] =
				*(fetch *)(&gf_ilog_d[fetchit*SOF]);
		}
	}
}

__device__ void gib_ggemm_d(int m, int n, int k,
			    shmem_bytes *A, int lda,
			    byte *B, int ldb,
			    unsigned char beta, shmem_bytes *C, int ldc) {
	/* Previous parameters: shmem_bytes *in_bufs, int buf_size) */
	/* Requirement:
	   lda % SOF == 0.  This prevents expensive divide operations.
	   Notes:
           - The 'm' parameter is explicit in the call. Unless this
             kernel is to check bounds on m, passing this parameter is
             not necessary.
	   - B is considered to be efficiently accessed in-place; no
             loading into faster areas is attempted. Access is not
             coordinated.
	   - This kernel is optimised for large m and small n, k.
	   - Compile time parameters: M (max n) and N (max k). Here,
             only M is used.
	   - There is a bug affecting CUDA compilers from version 2.3
	     onward that causes this kernel to miscompile for M=2. For
	     this case, there is some preprocessor trickiness that
	     allows this kernel to execute M=3, but only store for
	     M=2.
	*/

#if M == 2
#undef M
#define M 3
#define RAID6_FIX
#endif

	int rank = threadIdx.x + __umul24(blockIdx.x, blockDim.x);
	load_tables(threadIdx, blockDim);
	__syncthreads();

	shmem_bytes out[M];
	shmem_bytes in;

	if (beta == 0) {
		for (int i = 0; i < M; i++) {
			out[i].f = 0;
		}
	} else {
		for (int i = 0; i < n; i++) {
			out[i].f = C[rank + lda / SOF * i].f;
			int b_tmp = sh_log[beta];
			for (int b = 0; b < SOF; b++) {
				int sum = b_tmp + sh_log[(out[i].b)[b]];
				if (sum >= 255) sum -= 255;
				(out[i].b)[b] = sh_ilog[sum];
			}
		}
	}

	for (int i = 0; i < k; ++i) {
		in.f = A[rank + lda / SOF * i].f;
		for (int j = 0; j < n; ++j) {
			/* If I'm not hallucinating, this conditional really
			   helps on the 8800 stuff, but it hurts on the 260.
			*/
			//if (F_d[j*k+i] != 0) {
			int F_tmp = sh_log[B[j * ldb + i]];
			for (int b = 0; b < SOF; ++b) {
				if (in.b[b] != 0) {
					int sum_log = F_tmp +
						sh_log[(in.b)[b]];
					if (sum_log >= 255) sum_log -= 255;
					(out[j].b)[b] ^= sh_ilog[sum_log];
				}
			}
			//}
		}
	}

#ifdef RAID6_FIX
#undef M
#define M 2
#undef RAID6_FIX
#endif
	/* This works as long as buf_size % blocksize == 0 */
	for (int i = 0; i < n/*M*/; i++)
		C[rank + ldc / SOF * i].f = out[i].f;
}

__global__ void gib_generate_d(shmem_bytes *bufs, int buf_size)
{
	gib_ggemm_d(buf_size, M, N,
		    bufs, buf_size,
		    F_d, N,
		    0, bufs + buf_size * N / SOF, buf_size);
}

__global__ void gib_recover_d(shmem_bytes *bufs, int buf_size,
			       int recover_last)
{
	gib_ggemm_d(buf_size, recover_last, N,
		    bufs, buf_size,
		    F_d, N,
		    0, bufs + buf_size * N / SOF, buf_size);
}
