/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Shim for MediaTek's cpu_ctrl.h.  Mainline has no userlimit/dtram boost
 * framework; Wi-Fi throughput hints are simply ignored.  Not required for
 * the radio to associate, only for peak throughput.
 */
#ifndef _CPU_CTRL_H_
#define _CPU_CTRL_H_

#include <linux/types.h>

struct cpu_ctrl_data {
	int min;
	int max;
};

/* client ids */
#define CPU_KIR_WIFI	0

static inline int update_userlimit_cpu_freq(int id, int num,
					    struct cpu_ctrl_data *limits)
{
	return 0;
}

#endif /* _CPU_CTRL_H_ */
