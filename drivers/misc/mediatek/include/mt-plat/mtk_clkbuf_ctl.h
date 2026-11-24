/* SPDX-License-Identifier: GPL-2.0 */
/* Shim for MediaTek's mtk_clkbuf_ctl.h; mainline has no clock buffer driver. */
#ifndef _MTK_CLKBUF_CTL_H_
#define _MTK_CLKBUF_CTL_H_

#include <mtk-clkbuf-bridge.h>

enum mtk_clk_buf_id {
	MTK_CLK_BUF_NONE = 0,
	MTK_CLK_BUF_BT,
	MTK_CLK_BUF_WIFI,
	MTK_CLK_BUF_GPS,
	MTK_CLK_BUF_FM,
	MTK_CLK_BUF_MAX,
};

static inline void mtk_clkbuf_ctl(enum mtk_clk_buf_id id, bool onoff) { }
static inline void mtk_clkbuf_show_status_info(void) { }
static inline int mtk_clkbuf_get_xo_en_sta(int id) { return 0; }

#endif /* _MTK_CLKBUF_CTL_H_ */
