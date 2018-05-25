/* gib_cuda_driver.c: Host logic for CUDA
 *
 * Copyright (C) University of Alabama at Birmingham and Sandia
 * National Laboratories, 2010, written by Matthew L. Curry
 * <mlcurry@sandia.gov>, Rodrigo Sardinas <ras0054@tigermail.auburn.edu>
 * under contract to Sandia National Laboratories.
 *
 * Changes:
 * Initial version, Matthew L. Curry
 * Dec 16, 2014, Rodrigo Sardinas; revised to enable dynamic use.
 *
 */

/* TODO:
   - Noncontiguous only occurs on CPU!
*/

/* Size of each GPU buffer; n+m will be allocated (if mmap is not used). */
int gib_buf_size = 1024*1024;

const char env_error_str[] =
	"Your environment is not completely set. Please indicate a directory "
	"where\n Gibraltar kernel sources can be found. This should not be a "
	"publicly\naccessible directory.\n";

#include <gibraltar.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cuda_runtime_api.h>
#include <cuda.h>
#include <math.h>

#include "gib_context.h"
#include "gib_galois.h"
#include "gib_cpu_funcs.h"

int cudaInitialized = 0;

struct gpu_context_t {
	CUdevice dev;
	CUmodule module;
	CUcontext pCtx;
	CUfunction checksum;
	CUfunction recover_sparse;
	CUfunction recover;
	CUdeviceptr buffers;

	/* Flags controlled by user configuration */
	unsigned use_mmap; /* map host buffers into GPU instead of copying */
};

typedef struct gpu_context_t * gpu_context;

/* This macro checks for an error in the command given.  If it fails, the
 * entire program is killed.
 * TODO:  Fail over to CPU code if an error occurs.
 */
#define ERROR_CHECK_FAIL(cmd) {						\
		CUresult rc = cmd;					\
		if (rc != CUDA_SUCCESS) {				\
			fprintf(stderr, "%s failed with %i at "		\
				"%i in %s\n", #cmd, rc,			\
				__LINE__,  __FILE__);			\
			exit(EXIT_FAILURE);				\
		}							\
	}

/* If buffers are mapped into GPU, don't do memory copy. */
static int
memcpy_htod(CUdeviceptr dst, void *src, size_t n, gpu_context gpu_c)
{
	if (!gpu_c->use_mmap) {
		ERROR_CHECK_FAIL(cuMemcpyHtoD(dst, src, n));
	}
	return 0;
}

/* If buffers are mapped into GPU, don't do memory copy. */
static int
memcpy_dtoh(void *dst, CUdeviceptr src, size_t n, gpu_context gpu_c)
{
	if (!gpu_c->use_mmap) {
		ERROR_CHECK_FAIL(cuMemcpyDtoH(dst, src, n));
	}
	return 0;
}

/* Massive performance increases come from compiling the CUDA kernels
   specifically for the coding process at hand.  This does so with the
   following command line:
      nvcc --ptx -DN=n -DM=m src/gib_cuda_checksum.cu -o gib_cuda_n+m.ptx
   This is called in a separate process fork'd from the original.  This
   function should never return, and the parent process should wait on the
   return code from the compiler before resuming operation.
*/
void
gib_cuda_compile(int n, int m, char *filename)
{
	/* Never returns */
	char *executable = "nvcc";

	if (getenv("PATH") == NULL) {
		fprintf(stderr, "Your path is not set.  Please set it, and "
			"include the path to nvcc.");
		exit(1);
	}

	char argv1[100], argv2[100];
	sprintf(argv1, "-DN=%i", n);
	sprintf(argv2, "-DM=%i", m);
	if (getenv("GIB_SRC_DIR") == NULL) {
		fprintf(stderr, "%s", env_error_str);
		exit(1);
	}

	char src_filename[100];
	sprintf(src_filename, "%s/gib_cuda_checksum.cu",
		getenv("GIB_SRC_DIR"));
	char *const argv[] = {
		executable,
		"--ptx",
		argv1,
		argv2,
		src_filename,
		"-o",
		filename,
		NULL
	};

	execvp(argv[0], argv);
	perror("execve(nvcc)");
	fflush(0);
	exit(-1);
}

int
gib_init_cuda(int n, int m, struct gib_cuda_options *opts, gib_context *c)
{

	/* Initializes the CPU and GPU runtimes. */
	struct gib_cuda_options defaults = {
		.use_mmap = 1,
	};
	if (opts == NULL) {
		opts = &defaults;
	}

	static CUcontext pCtx;
	static CUdevice dev;
	if (m < 2 || n < 2) {
		fprintf(stderr, "It makes little sense to use Reed-Solomon "
			"coding when n or m is less than\ntwo. Use XOR or "
			"replication instead.\n");
		exit(1);
	}
	int rc_i = gib_cpu_init(n,m,c);
	if (rc_i != GIB_SUC) {
		fprintf(stderr, "gib_cpu_init returned %i\n", rc_i);
		exit(EXIT_FAILURE);
	}

	int gpu_id = 0;
	if (!cudaInitialized) {
		/* Initialize the CUDA runtime */
		int device_count;
		ERROR_CHECK_FAIL(cuInit(0));
		ERROR_CHECK_FAIL(cuDeviceGetCount(&device_count));
		if (getenv("GIB_GPU_ID") != NULL) {
			gpu_id = atoi(getenv("GIB_GPU_ID"));
			if (device_count <= gpu_id) {
				fprintf(stderr, "GIB_GPU_ID is set to an "
					"invalid value (%i).  There are only "
					"%i GPUs in the\n system.  Please "
					"specify another value.\n", gpu_id,
					device_count);
				exit(-1);
			}
		}
		cudaInitialized = 1;
	}
	ERROR_CHECK_FAIL(cuDeviceGet(&dev, gpu_id));

	unsigned ctx_flags = 0;
	if (opts->use_mmap)
		ctx_flags |= CU_CTX_MAP_HOST;
	ERROR_CHECK_FAIL(cuCtxCreate(&pCtx, ctx_flags, dev));

	/* Initialize the Gibraltar context */
	gpu_context gpu_c = (gpu_context)malloc(sizeof(struct gpu_context_t));
	gpu_c->dev = dev;
	gpu_c->pCtx = pCtx;
	gpu_c->use_mmap = opts->use_mmap;
	(*c)->acc_context = (void *)gpu_c;

	/* Determine whether the PTX has been generated or not by
	 * attempting to open it read-only.
	 */
	if (getenv("GIB_CACHE_DIR") == NULL) {
		fprintf(stderr, "%s", env_error_str);
		exit(-1);
	}

	/* Try to open the appropriate ptx file.  If it doesn't exist, compile a
	 * new one.
	 */
	int filename_len = strlen(getenv("GIB_CACHE_DIR")) +
		strlen("/gib_cuda_+.ptx") + log10(n)+1 + log10(m)+1 + 1;
	char *filename = (char *)malloc(filename_len);
	sprintf(filename, "%s/gib_cuda_%i+%i.ptx", getenv("GIB_CACHE_DIR"), n, m);

	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		/* Compile the ptx and open it */
		int pid = fork();
		if (pid == -1) {
			perror("Forking for nvcc");
			exit(-1);
		}
		if (pid == 0) {
			gib_cuda_compile(n, m, filename); /* never returns */
		}
		int status;
		wait(&status);
		if (status != 0) {
			printf("Waiting for the compiler failed.\n");
			printf("The exit status was %i\n",
			       WEXITSTATUS(status));
			printf("The child did%s exit normally.\n",
			       (WIFEXITED(status)) ? "" : " NOT");

			exit(-1);
		}
		fp = fopen(filename, "r");
		if (fp == NULL) {
			perror(filename);
			exit(-1);
		}
	}
	fclose(fp);

	/* If we got here, the ptx file exists.  Use it. */
	ERROR_CHECK_FAIL(cuModuleLoad(&(gpu_c->module), filename));
	ERROR_CHECK_FAIL(
		cuModuleGetFunction(&(gpu_c->checksum),
				    (gpu_c->module),
				    "_Z14gib_checksum_dP11shmem_bytesi"));
	ERROR_CHECK_FAIL(
		cuModuleGetFunction(&(gpu_c->recover),
				    (gpu_c->module),
				    "_Z13gib_recover_dP11shmem_bytesii"));

	/* Initialize the math libraries */
	gib_galois_init();
	unsigned char F[256*256];
	gib_galois_gen_F(F, m, n);

	/* Initialize/Allocate GPU-side structures */
	CUdeviceptr log_d, ilog_d, F_d;
	ERROR_CHECK_FAIL(cuModuleGetGlobal(&log_d, NULL, gpu_c->module,
					   "gf_log_d"));
	ERROR_CHECK_FAIL(cuMemcpyHtoD(log_d, gib_gf_log, 256));
	ERROR_CHECK_FAIL(cuModuleGetGlobal(&ilog_d, NULL, gpu_c->module,
					   "gf_ilog_d"));
	ERROR_CHECK_FAIL(cuMemcpyHtoD(ilog_d, gib_gf_ilog, 256));
	ERROR_CHECK_FAIL(cuModuleGetGlobal(&F_d, NULL, gpu_c->module, "F_d"));
	ERROR_CHECK_FAIL(cuMemcpyHtoD(F_d, F, m*n));
	if (gpu_c->use_mmap == 0) {
		ERROR_CHECK_FAIL(cuMemAlloc(&(gpu_c->buffers),
					    (n+m)*gib_buf_size));
	}

	ERROR_CHECK_FAIL(cuCtxPopCurrent((&gpu_c->pCtx)));
	free(filename);

	//set strategy for other functions
	(*c)->strategy = &cuda;

	return GIB_SUC;
}

static int
_gib_destroy(gib_context c)
{
	/* TODO:  Make sure everything created in gib_init is destroyed
	   here. */
	ERROR_CHECK_FAIL(
		cuCtxPushCurrent(((gpu_context)(c->acc_context))->pCtx));
	gpu_context gpu_c = (gpu_context) c->acc_context;
	if (gpu_c->use_mmap == 0) {
		ERROR_CHECK_FAIL(cuMemFree(gpu_c->buffers));
	}
	ERROR_CHECK_FAIL(cuModuleUnload(gpu_c->module));
	ERROR_CHECK_FAIL(cuCtxDestroy(gpu_c->pCtx));
	free(gpu_c);
	int rc_i = gib_cpu_destroy(c);
	if (rc_i != GIB_SUC) {
		printf("gib_cpu_destroy returned %i\n", rc_i);
		exit(EXIT_FAILURE);
	}
	return GIB_SUC;
}

static int
_gib_alloc(void **buffers, int buf_size, int *ld, gib_context c)
{
	ERROR_CHECK_FAIL(
		cuCtxPushCurrent(((gpu_context)(c->acc_context))->pCtx));
	gpu_context gpu_c = (gpu_context)(c->acc_context);
	unsigned alloc_flags = 0;
	if (gpu_c->use_mmap)
		alloc_flags |= CU_MEMHOSTALLOC_DEVICEMAP;
	ERROR_CHECK_FAIL(cuMemHostAlloc(buffers, (c->n + c->m) * buf_size,
					alloc_flags));
	*ld = buf_size;
	ERROR_CHECK_FAIL(
		cuCtxPopCurrent(&((gpu_context)(c->acc_context))->pCtx));
	return GIB_SUC;
}

static int
_gib_free(void *buffers, gib_context c)
{
	ERROR_CHECK_FAIL(
		cuCtxPushCurrent(((gpu_context)(c->acc_context))->pCtx));
	ERROR_CHECK_FAIL(cuMemFreeHost(buffers));
	ERROR_CHECK_FAIL(
		cuCtxPopCurrent(&((gpu_context)(c->acc_context))->pCtx));
	return GIB_SUC;
}

static int
_gib_generate(void *buffers, int buf_size, gib_context c)
{
	ERROR_CHECK_FAIL(
		cuCtxPushCurrent(((gpu_context)(c->acc_context))->pCtx));
	/* Do it all at once if the buffers are small enough */
	gpu_context gpu_c = (gpu_context)(c->acc_context);
	if (gpu_c->use_mmap == 0 && buf_size > gib_buf_size) {
		/* This is too large to do at once in the GPU memory we have
		 * allocated.  Split it into several noncontiguous jobs.
		 */
		int rc = gib_generate_nc(buffers, buf_size, buf_size, c);
		ERROR_CHECK_FAIL(cuCtxPopCurrent(&gpu_c->pCtx));
		return rc;
	}

	int nthreads_per_block = 128;
	int fetch_size = sizeof(int)*nthreads_per_block;
	int nblocks = (buf_size + fetch_size - 1)/fetch_size;

	unsigned char F[256*256];
	gib_galois_gen_F(F, c->m, c->n);
	CUdeviceptr F_d;
	ERROR_CHECK_FAIL(cuModuleGetGlobal(&F_d, NULL, gpu_c->module, "F_d"));
	ERROR_CHECK_FAIL(cuMemcpyHtoD(F_d, F, (c->m)*(c->n)));

	/* Copy the buffers to memory */
	memcpy_htod(gpu_c->buffers, buffers, (c->n) * buf_size, gpu_c);

	/* Configure and launch */
	ERROR_CHECK_FAIL(
		cuFuncSetBlockShape(gpu_c->checksum, nthreads_per_block, 1,
				    1));
	int offset = 0;
	void *ptr;

	if (gpu_c->use_mmap) {
		CUdeviceptr cpu_buffers;
		ERROR_CHECK_FAIL(cuMemHostGetDevicePointer(&cpu_buffers, buffers, 0));
		ptr = (void *)cpu_buffers;
	} else {
		ptr = (void *)(gpu_c->buffers);
	}

	ERROR_CHECK_FAIL(
		cuParamSetv(gpu_c->checksum, offset, &ptr, sizeof(ptr)));
	offset += sizeof(ptr);
	ERROR_CHECK_FAIL(
		cuParamSetv(gpu_c->checksum, offset, &buf_size,
			    sizeof(buf_size)));
	offset += sizeof(buf_size);
	ERROR_CHECK_FAIL(cuParamSetSize(gpu_c->checksum, offset));
	ERROR_CHECK_FAIL(cuLaunchGrid(gpu_c->checksum, nblocks, 1));

	/* Get the results back */
	if (gpu_c->use_mmap == 0) {
		CUdeviceptr tmp_d = gpu_c->buffers + c->n*buf_size;
		void *tmp_h = (void *)((unsigned char *)(buffers) + c->n*buf_size);
		memcpy_dtoh(tmp_h, tmp_d, (c->m) * buf_size, gpu_c);
	} else {
		ERROR_CHECK_FAIL(cuCtxSynchronize());
	}

	ERROR_CHECK_FAIL(
		cuCtxPopCurrent(&((gpu_context)(c->acc_context))->pCtx));
	return GIB_SUC;
}

static int
_gib_recover(void *buffers, int buf_size, int *buf_ids, int recover_last,
	    gib_context c)
{
	gpu_context gpu_c = (gpu_context)(c->acc_context);
	ERROR_CHECK_FAIL(cuCtxPushCurrent(gpu_c->pCtx));
	if (gpu_c->use_mmap && buf_size > gib_buf_size) {
		int rc = gib_cpu_recover(buffers, buf_size, buf_ids,
					 recover_last, c);
		ERROR_CHECK_FAIL(cuCtxPopCurrent(&gpu_c->pCtx));
		return rc;
	}

	int i, j;
	int n = c->n;
	int m = c->m;
	unsigned char A[256*256], inv[256*256], modA[256*256];
	for (i = n; i < n+recover_last; i++)
		if (buf_ids[i] >= n) {
			fprintf(stderr, "Attempting to recover a parity "
				"buffer, not allowed\n");
			return GIB_ERR;
		}

	gib_galois_gen_A(A, m+n, n);

	/* Modify the matrix to have the failed drives reflected */
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			modA[i*n+j] = A[buf_ids[i]*n+j];

	gib_galois_gaussian_elim(modA, inv, n, n);

	/* Copy row buf_ids[i] into row i */
	for (i = n; i < n+recover_last; i++)
		for (j = 0; j < n; j++)
			modA[i*n+j] = inv[buf_ids[i]*n+j];

	int nthreads_per_block = 128;
	int fetch_size = sizeof(int)*nthreads_per_block;
	int nblocks = (buf_size + fetch_size - 1)/fetch_size;

	CUdeviceptr F_d;
	ERROR_CHECK_FAIL(cuModuleGetGlobal(&F_d, NULL, gpu_c->module, "F_d"));
	ERROR_CHECK_FAIL(cuMemcpyHtoD(F_d, modA+n*n, (c->m)*(c->n)));

	memcpy_htod(gpu_c->buffers, buffers, c->n * buf_size, gpu_c);
	ERROR_CHECK_FAIL(cuFuncSetBlockShape(gpu_c->recover,
					     nthreads_per_block, 1, 1));
	int offset = 0;
	void *ptr;
	if (gpu_c->use_mmap) {
		CUdeviceptr cpu_buffers;
		ERROR_CHECK_FAIL(cuMemHostGetDevicePointer(&cpu_buffers, buffers, 0));
		ptr = (void *)cpu_buffers;
	} else {
		ptr = (void *)gpu_c->buffers;
	}
	ERROR_CHECK_FAIL(cuParamSetv(gpu_c->recover, offset, &ptr,
				     sizeof(ptr)));
	offset += sizeof(ptr);
	ERROR_CHECK_FAIL(cuParamSetv(gpu_c->recover, offset, &buf_size,
				     sizeof(buf_size)));
	offset += sizeof(buf_size);
	ERROR_CHECK_FAIL(cuParamSetv(gpu_c->recover, offset, &recover_last,
				     sizeof(recover_last)));
	offset += sizeof(recover_last);
	ERROR_CHECK_FAIL(cuParamSetSize(gpu_c->recover, offset));
	ERROR_CHECK_FAIL(cuLaunchGrid(gpu_c->recover, nblocks, 1));
	if (gpu_c->use_mmap == 0) {
		CUdeviceptr tmp_d = gpu_c->buffers + c->n*buf_size;
		void *tmp_h = (void *)((unsigned char *)(buffers) + c->n*buf_size);
		memcpy_dtoh(tmp_h, tmp_d, recover_last * buf_size, gpu_c);
	} else {
		cuCtxSynchronize();
	}

	ERROR_CHECK_FAIL(
		cuCtxPopCurrent(&((gpu_context)(c->acc_context))->pCtx));
	return GIB_SUC;
}

/* The inclusion of memory mapping has obviated the need for this before
   it was implemented.  It's done in the CPU to make it work, but there is
   no attempt to make it fast as there appears to be little need.  A GPU
   upgrade fixes this.

   TODO:  The MMapped version can benefit from this if the buffer isn't full.
   Bring this to life for that implementation only.
 */
static int
_gib_generate_nc(void *buffers, int buf_size, int work_size,
		gib_context c)
{
	return gib_cpu_generate_nc(buffers, buf_size, work_size, c);
}

static int
_gib_recover_nc(void *buffers, int buf_size, int work_size, int *buf_ids,
		int recover_last, gib_context c)
{
	return gib_cpu_recover_nc(buffers, buf_size, work_size, buf_ids,
				  recover_last, c);
}


struct dynamic_fp cuda = {
		.gib_alloc = &_gib_alloc,
		.gib_destroy = &_gib_destroy,
		.gib_free = &_gib_free,
		.gib_generate = &_gib_generate,
		.gib_generate_nc = &_gib_generate_nc,
		.gib_recover = &_gib_recover,
		.gib_recover_nc = &_gib_recover_nc,
};

