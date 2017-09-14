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

/*
 * MSRs
 */
#define IA32_APERF		0xe8
#define IA32_MPERF		0xe7
#define IA32_PERF_FIXED_CTR1	0x30a
#define IA32_FIXED_CTR_CTRL	0x38d
#define IA32_PERF_GLOBAL_CTRL	0x38f

/*
 * Fields of the IA32_FIXED_CTR_CTRL MSR
 */
/* Enable couting in ring 0 */
#define EN1_OS		1 << 4
/* Enable counting in higher rings */
#define EN1_USR		1 << 5
/* Enable counting for all core threads (if any) */
#define EN1_ANYTHR	1 << 6

/*
 * Fields of the IA32_PERF_GLOBAL_CTRL MSR
 */
/* Enable IA32_PERF_FIXED_CTR1 */
#define EN_FIXED_CTR1	1 << 1  /* in the top 32-bits */

/* CPUID leaves */
#define	CPUID_ARCH_PERF_CTRS		0x0a

/*
 * Fields of EAX after querying CPUID_ARCH_PERF_CTRS
 */
/* Architectural performance counters version */
#define CPUID_ARCH_PERF_CTRS_VERS	0xff

/*
 * Fields of EDX after querying CPUID_ARCH_PERF_CTRS
 */
/* Fixed-function performance counters bit-width */
#define CPUID_FIXED_PERF_CTRS_WIDTH	0x1fe0
