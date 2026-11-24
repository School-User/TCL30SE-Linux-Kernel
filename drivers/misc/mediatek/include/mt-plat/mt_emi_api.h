/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Shim for MediaTek's mt_emi_api.h (EMI MPU / DRAM remapping).
 * TODO: the real driver programs EMI MPU regions so that connsys DMA can
 * access its reserved memory; on mainline this has to be reimplemented
 * (or replaced by a reserved-memory + no-MPU setup).  Values are placeholders
 * sufficient to compile the port.
 */
#ifndef _MT_EMI_API_H_
#define _MT_EMI_API_H_

#include <linux/types.h>

struct EMI_CTRL_STATE_OFFSET {
	unsigned int emi_apmem_ctrl_state;
	unsigned int emi_apmem_ctrl_host_sync_state;
	unsigned int emi_apmem_ctrl_host_sync_num;
	unsigned int emi_apmem_ctrl_chip_sync_state;
	unsigned int emi_apmem_ctrl_chip_sync_num;
	unsigned int emi_apmem_ctrl_chip_sync_addr;
	unsigned int emi_apmem_ctrl_chip_sync_len;
	unsigned int emi_apmem_ctrl_chip_print_buff_start;
	unsigned int emi_apmem_ctrl_chip_print_buff_len;
	unsigned int emi_apmem_ctrl_chip_print_buff_idx;
};

/* AP-side control word offsets inside the reserved connsys region */
#define EXP_APMEM_CTRL_STATE			0x0
#define EXP_APMEM_CTRL_HOST_SYNC_STATE		0x4
#define EXP_APMEM_CTRL_HOST_SYNC_NUM		0x8
#define EXP_APMEM_CTRL_CHIP_SYNC_STATE		0xC
#define EXP_APMEM_CTRL_CHIP_SYNC_NUM		0x10
#define EXP_APMEM_CTRL_CHIP_SYNC_ADDR		0x14
#define EXP_APMEM_CTRL_CHIP_SYNC_LEN		0x18
#define EXP_APMEM_CTRL_CHIP_PRINT_BUFF_START	0x1C
#define EXP_APMEM_CTRL_CHIP_PRINT_BUFF_LEN	0x20
#define EXP_APMEM_CTRL_CHIP_PRINT_BUFF_IDX	0x24

#define EMI_MPU_REGION_MAX	32

static inline int mt_emi_mpu_set_protection(unsigned int start,
					    unsigned int end,
					    unsigned int ap_perm,
					    unsigned int md_perm)
{
	return 0;
}

#endif /* _MT_EMI_API_H_ */
