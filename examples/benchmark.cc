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
#include <limits.h>

using namespace std;

enum actions {
	encode = 0,
	decode,
	last_action,
};

const char *action_names[] = {
	"encode",
	"decode",
};

enum backends {
	cpu = 0,
	jerasure,
	cuda,
	last_backend,
};

const char *backend_names[] = {
	"cpu",
	"jerasure",
	"cuda",
};

struct opts {
	int k;
	int m;
	unsigned niters;
	int action;
	int backend;
	const char *cache_dir;
	const char *src_dir;
};

void
print_usage(const char *progname)
{
	fprintf(stderr, "Usage: %s [OPTIONS]\n", progname);
	fprintf(stderr, "Reports an average benchmark time.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Mandatory options:\n");
	fprintf(stderr, "    -k: Number of data buffers\n");
	fprintf(stderr, "    -m: Number of coding buffers\n");
	fprintf(stderr, "    -b: Backend to benchmark. "
		"Available backends:\n");
	for (int i = 0; i < last_backend; i++) {
		printf("        %s\n", backend_names[i]);
	}
	fprintf(stderr, "Other options:\n");
	fprintf(stderr, "    -i: Number of iterations (Default: 1)\n");
	fprintf(stderr, "    -a: Action to benchmark (Default: %s). ",
		action_names[0]);
	fprintf(stderr, "Available actions:\n");
	for (int i = 0; i < last_action; i++) {
		printf("        %s\n", action_names[i]);
	}
	fprintf(stderr, "Mandatory CUDA options:\n");
	fprintf(stderr, "    -c: Path to store compiled kernels\n");
	fprintf(stderr, "    -s: Path to kernel source\n");
}

static int
strtou(char *str, unsigned *out)
{
	/* Returns zero if no error is encountered. The string must
	 * precisely map to an unsigned integer. */
	long long x;
	char *end;
	x = strtoll(str, &end, 10);
	if (end != '\0' && end != str && x < UINT_MAX) {
		*out = (unsigned)x;
		return 0;
	}
	return -1;
}

void
parse_args(int argc, char **argv, struct opts *opts)
{
	const char *progname = argv[0];

	opts->k = -1;
	opts->m = -1;
	opts->niters = 1;
	opts->action = 0;
	opts->backend = last_backend;
	opts->cache_dir = NULL;
	opts->src_dir = NULL;

	int opt;
	unsigned u;
	while ((opt = getopt(argc, argv, "k:m:i:hb:a:c:s:")) != -1) {
		switch (opt) {
		case 'b':
			for (int i = 0; i < last_backend; i++) {
				if (strcmp(backend_names[i], optarg) == 0) {
					opts->backend = i;
				}
			}
			if (opts->backend == last_backend) {
				printf("Invalid backend: %s\n", optarg);
				print_usage(progname);
				exit(EXIT_FAILURE);
			}
			break;
		case 'a':
			for (int i = 0; i < last_action; i++) {
				if (strcmp(action_names[i], optarg) == 0) {
					opts->action = i;
				}
			}
			if (opts->action == last_action) {
				printf("Invalid action: %s\n", optarg);
				print_usage(progname);
				exit(EXIT_FAILURE);
			}
			break;
		case 'k':
		case 'm':
		case 'i':
			/* Small positive integer options */
			if (strtou(optarg, &u) || u > INT_MAX) {
				fprintf(stderr, "Invalid value for -%c: %s\n",
					opt, optarg);
				print_usage(progname);
				exit(EXIT_FAILURE);
			}
			if (opt == 'k') {
				opts->k = (int)u;
			} else if (opt == 'm') {
				opts->m = (int)u;
			} else if (opt == 'i') {
				opts->niters = (int)u;
			}
			break;
		case 'h':
			print_usage(progname);
			exit(EXIT_SUCCESS);
		case 'c':
			opts->cache_dir = optarg;
			break;
		case 's':
			opts->src_dir = optarg;
			break;
		default:
			print_usage(progname);
			exit(EXIT_FAILURE);
		}
	}

	if (opts->k == -1 || opts->m == -1 || opts->backend == last_backend) {
		fprintf(stderr, "Required options missing.\n");
		print_usage(progname);
		exit(EXIT_FAILURE);
	}

	if (opts->backend == cuda) {
		if (opts->cache_dir == NULL || opts->src_dir == NULL) {
			fprintf(stderr, "Required CUDA options missing\n");
			print_usage(progname);
			exit(EXIT_FAILURE);
		}
	}

	if (opts->k >= 256 || opts->m >= 256 || opts->k + opts->m > 256) {
		fprintf(stderr, "k+m must be 256 or less.\n");
		print_usage(progname);
		exit(EXIT_FAILURE);
	}
}

double
etime(void)
{
	/* Return time since epoch (in seconds) */
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec + 1.e-6*t.tv_usec;
}

#define time_iters(var, cmd, iters) do {				\
		(var) = -etime();					\
		for (int iter = 0; iter < (iters); iter++) {		\
			cmd;						\
		}							\
		(var) += etime();					\
	} while(0)

int
main(int argc, char **argv)
{
	struct opts opts;
	parse_args(argc, argv, &opts);
	int iters = opts.niters;
	int n = opts.k;
	int m = opts.m;

	double op_time;
	double tmptime;

	struct gib_cuda_options cuda_opts;
	cuda_opts.use_mmap = 1;
	cuda_opts.kernel_src_dir = opts.src_dir;
	cuda_opts.kernel_cache_dir = opts.cache_dir;

	int rc;
	gib_context_t * gc;

	switch(opts.backend) {
	case jerasure:
		rc = gib_init_jerasure(n, m, &gc);
		break;
	case cpu:
		rc = gib_init_cpu(n, m, &gc);
		break;
	case cuda:
		rc = gib_init_cuda(n, m, &cuda_opts, &gc);
		break;
	default:
		printf("Invalid backend ID (opts.backend)\n");
		abort();
	}

	if (rc) {
		printf("Error:  %i\n", rc);
		exit(EXIT_FAILURE);
	}

	int size = 1024 * 1024;
	void *data;
	gib_alloc(&data, size, &size, gc);

	for (int i = 0; i < size * n; i++)
		((char *) data)[i] = (unsigned char) rand() % 256;

	time_iters(opts.action == encode ? op_time : tmptime,
		   gib_generate(data, size, gc),
		   opts.action == encode ? iters : 1);

	unsigned char *backup_data = (unsigned char *)malloc(size * (n + m));

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

		/* Destroy the buffer contents */
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
	memset((unsigned char *) dense_data + n * size, 0, size * nfailed);
	time_iters(opts.action == decode ? op_time : tmptime,
		   gib_recover(dense_data, size, buf_ids, nfailed, gc),
		   opts.action == decode ? iters : 1);

	for (int i = 0; i < m + n; i++) {
		if (memcmp((unsigned char *) dense_data + i * size,
			   backup_data + buf_ids[i] * size, size)) {
			printf("Dense test failed on buffer %i/%i.\n", i,
			       buf_ids[i]);
			exit(1);
		}
	}

	double size_mb = size * n / 1024.0 / 1024.0;

	printf("%lf\n", op_time / opts.niters);

	gib_free(data, gc);
	gib_free(dense_data, gc);
	free(backup_data);
	gib_destroy(gc);

	return 0;
}
