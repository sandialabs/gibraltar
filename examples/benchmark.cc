/* Author:  Matthew L. Curry
 * Email:   mlcurry@sandia.gov
 *
 * Simple benchmarking application.  Sets up an n+m configuration, fails m data
 * buffers (in order to put Jerasure on even footing, as Gibraltar does not
 * automatically recover checksum buffers and Jerasure does), and recovers
 * them.  Results are verified correct.
 *
 * This is an example of an application that does not particularly care about
 * data layout for regeneration.  For an application that is mindful of this,
 * see the other example.
 */

#include <gibraltar.h>
#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <cstring>
#include <cstdio>
using namespace std;

#ifndef min_test
#define min_test 2
#endif
#ifndef max_test
#define max_test 16
#endif

double
etime(void)
{
	/* Return time since epoch (in seconds) */
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec + 1.e-6*t.tv_usec;
}

#define time_iters(var, cmd, iters) do {				\
		var = -1*etime();					\
		for (int iter = 0; iter < iters; iter++)		\
			cmd;						\
		var = (var + etime()) / iters;				\
	} while(0)

int
main(int argc, char **argv) {
	int iters = 5;
	printf("%% Speed test with correctness checks\n");
	printf("%% datasize is n*bufsize, or the total size of all data buffers\n");
	printf("%%      n        m datasize chk_tput rec_tput\n");

	for (int m = min_test; m <= max_test; m++) {
		for (int n = min_test; n <= max_test; n++) {
			double chk_time, dns_time;
			printf("%8i %8i ", n, m);
			gib_context gc;
			int rc = gib_init(n, m, &gc);
			if (rc) {
				printf("Error:  %i\n", rc);
				exit(EXIT_FAILURE);
			}

			/* Allocate/define the appropriate number of buffers */
			int size = 1024*1024;
			void *data;
			gib_alloc(&data, size, &size, gc);

			for (int i = 0; i < size * n; i++)
				((char *)data)[i] = (unsigned char)rand()%256;

			time_iters(chk_time, gib_generate(data, size, gc), iters);

			unsigned char *backup_data = (unsigned char *)malloc(size * (n+m));
			memcpy(backup_data, data, size * (n+m));

			char failed[256];
			for (int i = 0; i < n+m; i++)
				failed[i] = 0;
			for (int i = 0; i < ((m < n) ? m : n); i++) {
				int probe;
				do {
					probe = rand() % n;
				} while (failed[probe] == 1);
				failed[probe] = 1;

				/* Destroy the buffer */
				memset((char *)data + size*probe, 0, size);
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
			while (f_index != n+m) {
				buf_ids[f_index] = f_index;
				f_index++;
			}

			void *dense_data;
			gib_alloc((void **)&dense_data, size, &size, gc);
			for (int i = 0; i < m+n; i++) {
				memcpy((unsigned char *)dense_data+i*size,
				       (unsigned char *)data+buf_ids[i]*size, size);
			}

			int nfailed = (m < n) ? m : n;
			memset((unsigned char *)dense_data+n*size, 0, size*nfailed);
			time_iters(dns_time, gib_recover(dense_data, size, buf_ids, nfailed, gc),
				   iters);

			for (int i = 0; i < m+n; i++) {
				if (memcmp((unsigned char *)dense_data+i*size,
					   backup_data+buf_ids[i]*size, size)) {
					printf("Dense test failed on buffer %i/%i.\n", i,
					       buf_ids[i]);
					exit(1);
				}
			}

			double size_mb = size * n / 1024.0 / 1024.0;
			printf("%8i %8.3lf %8.3lf\n", size*n, size_mb/chk_time,
			       size_mb/dns_time);

			gib_free(data, gc);
			gib_free(dense_data, gc);
			free(backup_data);
			gib_destroy(gc);
		}
	}
	return 0;
}
