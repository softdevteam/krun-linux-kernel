/*
 * System calls for low latency MSR reads.
 *
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

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/smp.h>
#include <asm/msr.h>
#include <linux/uaccess.h>

#include "krun_reg.h"

/* protos */
void krun_read_msrs(void *data);
void krun_reset_msrs(void *unused);
asmlinkage int sys_krun_read_msrs(int n_cores, bool ctr1_first, u64 *aperfs,
    u64 *mperfs, u64 *ctr1s);
asmlinkage void sys_krun_reset_msrs(int n_cores);
asmlinkage int sys_krun_configure(int n_cores);
u8 krun_get_arch_perf_ctr_version(void);

/* For passing args to krun_read_msrs() */
struct krun_msr_args {
	u64 aperf;
	u64 mperf;
	u64 ctr1;
	bool ctr1_first;
};

/*
 * Take low-latency MSR measurements for the current CPU core.
 */
void krun_read_msrs(void *data)
{
	u32 hi, lo;
	struct krun_msr_args *args = (struct krun_msr_args *) data;

	if (args->ctr1_first) {
		asm volatile(
		    "rdmsr\n\t"
		    : "=d" (hi), "=a" (lo)	/* out */
		    : "c"(IA32_PERF_FIXED_CTR1) /* in */
		    :);				/* clobber */
		args->ctr1 = ((u64) hi << 32) | lo;
	}

	asm volatile(
	    "rdmsr\n\t"
	    : "=d" (hi), "=a" (lo)	/* out */
	    : "c"(IA32_APERF)		/* in */
	    :);				/* clobber */
	args->aperf = ((u64) hi << 32) | lo;

	asm volatile(
	    "rdmsr\n\t"
	    : "=d" (hi), "=a" (lo)	/* out */
	    : "c"(IA32_MPERF)		/* in */
	    : );			/* clobber */
	args->mperf = ((u64) hi << 32) | lo;

	if (!args->ctr1_first) {
		asm volatile(
		    "rdmsr\n\t"
		    : "=d" (hi), "=a" (lo)	/* out */
		    : "c"(IA32_PERF_FIXED_CTR1) /* in */
		    :);				/* clobber */
		args->ctr1 = ((u64) hi << 32) | lo;
	}
}

/* Reset the MSRs of interest on the current CPU core */
void krun_reset_msrs(void *unused)
{
	/*
	 * Note we reset MPERF before APERF, so that APERF can never be greater
	 * than MPERF.
	 */
	asm volatile(
	    "xor %%eax, %%eax\n\t"
	    "xor %%edx, %%edx\n\t"
	    "mov %0, %%ecx\n\t"
	    "wrmsr\n\t"
	    "mov %1, %%ecx\n\t"
	    "wrmsr\n\t"
	    "mov %2, %%ecx\n\t"
	    "wrmsr\n\t"
	    : 					/* out */
	    : "i"(IA32_MPERF), "i"(IA32_APERF),
	      "i"(IA32_PERF_FIXED_CTR1)		/* in */
	    :"eax", "ecx", "edx");		/* clobber */
}

u8 krun_get_arch_perf_ctr_version()
{
	u32 eax;

	asm volatile(
	    "mov %1, %%eax\n\t"
	    "cpuid\n\t"
	    : "=a" (eax)			/* out */
	    : "i"(CPUID_ARCH_PERF_CTRS) 	/* in */
	    :"ebx", "ecx", "edx");		/* clobber */

	return eax & CPUID_ARCH_PERF_CTRS_VERS;
}

/* ------------------------
 * System call entry points
 * ------------------------
 */

/*
 * Read the MSRs for the first n_cores cores and return the values via the
 * `aperfs', `mperfs' and `ctr1s' arrays.
 *
 * The arrays should have been pre-allocated by userspace with enough
 * space for one 64-bit unsigned integer per-core.
 *
 * If `ctr1_first' is true, then read the IA32_PERF_FIXED_CTR1 first, otherwise
 * it is read last.
 *
 * Assumes no CPU cores have been offlined.
 *
 * Returns non-zero on error.
 */
asmlinkage int sys_krun_read_msrs(int n_cores, bool ctr1_first, u64 *aperfs,
    u64 *mperfs, u64 *ctr1s)
{
	struct krun_msr_args args;
	unsigned long err;
	int core;

	args.ctr1_first = ctr1_first;
	for (core = 0; core < n_cores; core++) {
		/* Take readings */
		smp_call_function_single(core, krun_read_msrs, &args, 1);

		/* Copy results from kernel memory into userspace virtual memory */
		err = copy_to_user(&aperfs[core], &args.aperf, sizeof(u64));
		if (err != 0) {
			printk("copy_to_user failed with %lu\n", err);
			return -EFAULT;
		}
		err = copy_to_user(&mperfs[core], &args.mperf, sizeof(u64));
		if (err != 0) {
			printk("copy_to_user failed with %lu\n", err);
			return -EFAULT;
		}
		err = copy_to_user(&ctr1s[core], &args.ctr1, sizeof(u64));
		if (err != 0) {
			printk("copy_to_user failed with %lu\n", err);
			return -EFAULT;
		}
	}

	return 0;
}

/*
 * Reset the MSRs for the first n_cores cores.
 */
asmlinkage void sys_krun_reset_msrs(int n_cores) {
	int core;
	for (core = 0; core < n_cores; core++) {
		smp_call_function_single(core, krun_reset_msrs, (void *) NULL, 1);
	}
}

/*
 * Configure the fixed-function performance counter on the first n_cores cores.
 *
 * Returns non-zero on error.
 */
asmlinkage int sys_krun_configure(int n_cores) {
	/*
	 * Since we dont mind about latency here, we may as well use the helper
	 * functions provided by <asm/msr.h>.
	 */
	u32 hi, lo;
	int err, core;
	u8 ctrs_version;

	ctrs_version = krun_get_arch_perf_ctr_version();
	for (core = 0; core < n_cores; core++) {
		/* Turn on the counter */
		err = rdmsr_safe_on_cpu(core, IA32_PERF_GLOBAL_CTRL, &lo, &hi);
		if (err) {
			printk("problem reading IA32_PERF_GLOBAL_CTRL\n");
			return err;
		}
		hi |= EN_FIXED_CTR1;

		err = wrmsr_safe_on_cpu(core, IA32_PERF_GLOBAL_CTRL, lo, hi);
		if (err) {
			printk("problem writing IA32_PERF_GLOBAL_CTRL\n");
			return err;
		}

		/* Define what is being counted */
		err = rdmsr_safe_on_cpu(core, IA32_FIXED_CTR_CTRL, &lo, &hi);
		if (err) {
			printk("problem reading IA32_FIXED_CTR_CTRL\n");
			return err;
		}

		/* All of the flags are in the lower 32-bits of the MSR */
		lo |= (EN1_OS | EN1_USR);
		if (ctrs_version >= 3) {
			/* ANYTHR flag available only after performance counters v3 */
			lo |= EN1_ANYTHR;
		}

		err = wrmsr_safe_on_cpu(core, IA32_FIXED_CTR_CTRL, lo, hi);
		if (err) {
			printk("problem writing IA32_FIXED_CTR_CTRL\n");
			return err;
		}
	}
	return 0;
}
