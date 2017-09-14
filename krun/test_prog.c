/*
 * Copyright (C) 2017, King's College London.
 * Created by the Software Development Team <http://soft-dev.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <err.h>
#include <asm/unistd.h>

#include "krun_reg.h"

/*
 * For testing when you don't have the libc-dev headers installed, you can
 * uncomment these macros. These must stay in-sync with the syscall table.
 */
#if 0
#define __NR_krun_read_msrs	332
#define __NR_krun_reset_msrs	333
#define __NR_krun_configure	334
#endif

/* protos */
void read_msrs_checked(int n_cores, bool ctr1_first, u_int64_t *aperfs,
    u_int64_t *mperfs, u_int64_t *ctr1s);
u_int64_t **alloc_array(int n_cores);
void print_arrays(int n_cores, u_int64_t **aperfs, u_int64_t **mperfs, u_int64_t **ctr1s);
int get_ctr1_width(void);
void mask_ctr1s(int n_cores, u_int64_t **ctr1s);

/* ctr1 is variable size */
u_int64_t ctr1_mask = 0;

int get_ctr1_width(void) {
	uint32_t eax, edx;
	int width;

	asm volatile(
	    "mov %2, %%eax\n\t"
	    "cpuid\n\t"
	    : "=a" (eax), "=d" (edx)    /* out */
	    : "i"(CPUID_ARCH_PERF_CTRS) /* in*/
	    : "ebx", "ecx");            /* clobber */
	width = (edx & CPUID_FIXED_PERF_CTRS_WIDTH) >> 5;
	printf("ctr1 width is %d\n", width);
	return width;
}

void read_msrs_checked(int n_cores, bool ctr1_first, u_int64_t *aperfs,
    u_int64_t *mperfs, u_int64_t *ctr1s)
{
	int rv = syscall(__NR_krun_read_msrs, n_cores, ctr1_first, aperfs,
	    mperfs, ctr1s);
	if (rv != 0) {
		err(EXIT_FAILURE, "krun_read_msrs failed");
	}
}

/* allocates space for 2 readings of one performance counter for all cores */
u_int64_t **alloc_array(int n_cores)
{
	u_int64_t **arr;
	int i;

	arr = calloc(2, sizeof(u_int64_t *));
	if (arr == NULL) {
		errx(EXIT_FAILURE, "calloc");
	}

	for (i = 0; i < 2; i++) {
		arr[i] = calloc(n_cores, sizeof(u_int64_t));
		if (arr[i] == NULL) {
			errx(EXIT_FAILURE, "calloc");
		}
	}
	return arr;
}

/* apply masking to ctr1s */
void mask_ctr1s(int n_cores, u_int64_t **ctr1s)
{
	int core;

	for (core = 0; core < n_cores; core++) {
		ctr1s[0][core] &= ctr1_mask;
		ctr1s[1][core] &= ctr1_mask;
	}
}

/* print before and after readings for all counters */
void print_arrays(int n_cores, u_int64_t **aperfs, u_int64_t **mperfs,
    u_int64_t **ctr1s)
{
	int core, idx;

	for (idx = 0; idx < 2; idx++) {
		if (!idx) {
			printf("Before:\n");
		} else {
			printf("After:\n");
		}
		for (core = 0; core < n_cores; core++) {
			printf("  core: %02d: aperf: %016" PRIu64 "    ",
			    core, aperfs[idx][core]);
			printf("mperf: %016" PRIu64 "    ", mperfs[idx][core]);
			printf("ctr1 : %016" PRIu64 "\n", ctr1s[idx][core]);

			/* check they make sense too */
			if (aperfs[0][core] > aperfs[1][core]) {
				err(EXIT_FAILURE, "bad aperfs");
			}
			if (mperfs[0][core] > mperfs[1][core]) {
				err(EXIT_FAILURE, "bad mperfs");
			}
			if (ctr1s[0][core] > ctr1s[1][core]) {
				err(EXIT_FAILURE, "bad ctr1");
			}
		}
	}
}

int main(void)
{
	u_int64_t **aperfs, **mperfs, **ctr1s;
	int n_cores;

	ctr1_mask = ((u_int64_t) 1 << get_ctr1_width()) - 1;
	printf("ctr1 mask is %" PRIx64 "\n", ctr1_mask);
	n_cores = sysconf(_SC_NPROCESSORS_ONLN);

	/* make space for readings */
	aperfs = alloc_array(n_cores);
	mperfs = alloc_array(n_cores);
	ctr1s = alloc_array(n_cores);

	printf("configuring for %d cores\n", n_cores);
	syscall(__NR_krun_configure, n_cores);
	syscall(__NR_krun_reset_msrs, n_cores);

	/* continually read MSRs */
	while (true) {
		printf("Sampling MSRs. Press CTRL+C to stop.\n");
		read_msrs_checked(n_cores, false, aperfs[0], mperfs[0], ctr1s[0]);
		read_msrs_checked(n_cores, true, aperfs[1], mperfs[1], ctr1s[1]);
		mask_ctr1s(n_cores, ctr1s);
		print_arrays(n_cores, aperfs, mperfs, ctr1s);
		usleep(1 * 1000 * 500);
		printf("\n");
	}
	/* unreachable */
	return 0;
}
