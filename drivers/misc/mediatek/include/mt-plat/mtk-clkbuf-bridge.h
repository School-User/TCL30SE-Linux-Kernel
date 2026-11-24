/* SPDX-License-Identifier: GPL-2.0 */
/* Shim for MediaTek clock buffer bridge (no clock buffer on mainline). */
#ifndef _MTK_CLKBUF_BRIDGE_H_
#define _MTK_CLKBUF_BRIDGE_H_

#include <linux/types.h>

/* XO / clock buffer users; only the identifiers are needed to compile. */
enum clk_buf_id {
	CLK_BUF_BT = 0,
	CLK_BUF_WIFI,
	CLK_BUF_GPS,
	CLK_BUF_FM,
	CLK_BUF_CONN,
	CLK_BUF_MAX,
};

#define clk_buf_ctrl(id, onoff)		do { } while (0)
#define clk_buf_show_status_info()	do { } while (0)
#define clk_buf_get_xo_en_sta(id)	(0)

#endif /* _MTK_CLKBUF_BRIDGE_H_ */
