/* SPDX-License-Identifier: GPL-2.0 */
/* Shim: MediaTek topology control driver does not exist on mainline. */
#ifndef _TOPO_CTRL_H_
#define _TOPO_CTRL_H_

static inline int topo_ctrl_get_nr_clusters(void)
{
	return 1;
}

#endif /* _TOPO_CTRL_H_ */
