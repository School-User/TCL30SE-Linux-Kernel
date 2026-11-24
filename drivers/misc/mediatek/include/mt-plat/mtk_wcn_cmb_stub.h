/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Shim for MediaTek's mtk_wcn_cmb_stub.h (mt-plat, 4.19 ALPS kernel).
 * The connectivity tree provides its own implementations
 * (common/common_detect/mtk_wcn_stub_alps.c); only the declarations are needed.
 *
 * NOTE: prototype/field signatures were reconstructed from the call sites in
 * mtk-connectivity; revisit if the compiler reports a mismatch.
 */
#ifndef _MTK_WCN_CMB_STUB_H_
#define _MTK_WCN_CMB_STUB_H_

#include <linux/types.h>

/* Audio interface states used for BT/Wi-Fi PCM sharing */
enum CMB_STUB_AIF_X {
	CMB_STUB_AIF_0 = 0,
	CMB_STUB_AIF_1,
	CMB_STUB_AIF_2,
	CMB_STUB_AIF_3,
	CMB_STUB_AIF_4,
	CMB_STUB_AIF_MAX,
};

enum CMB_STUB_AIF_CTRL {
	CMB_STUB_AIF_CTRL_EN = 0,
	CMB_STUB_AIF_CTRL_DIS,
	CMB_STUB_AIF_CTRL_MAX,
};

/* change notification types for mtk_wcn_cmb_stub_func_ctrl() */
#define CMB_STUB_FUNC_BT	0
#define CMB_STUB_FUNC_FM	1
#define CMB_STUB_FUNC_GPS	2
#define CMB_STUB_FUNC_WIFI	3

typedef int (*wmt_aif_ctrl_cb)(enum CMB_STUB_AIF_X state,
			       enum CMB_STUB_AIF_CTRL ctrl);
typedef void (*wmt_func_ctrl_cb)(unsigned int type, unsigned int on);
typedef long (*wmt_thermal_query_cb)(void);
typedef int (*wmt_trigger_assert_cb)(void);
typedef int (*wmt_deep_idle_ctrl_cb)(unsigned int enter);
typedef int (*wmt_func_do_reset)(unsigned int type);
typedef void (*wmt_clock_fail_dump_cb)(void);

struct _CMB_STUB_CB_ {
	unsigned int size;
	wmt_aif_ctrl_cb aif_ctrl_cb;
	wmt_func_ctrl_cb func_ctrl_cb;
	wmt_thermal_query_cb thermal_query_cb;
	wmt_trigger_assert_cb trigger_assert_cb;
	wmt_deep_idle_ctrl_cb deep_idle_ctrl_cb;
	wmt_func_do_reset wmt_do_reset_cb;
	wmt_clock_fail_dump_cb clock_fail_dump_cb;
};

int mtk_wcn_cmb_stub_reg(struct _CMB_STUB_CB_ *p_stub_cb);
int mtk_wcn_cmb_stub_unreg(void);
int mtk_wcn_cmb_stub_aif_ctrl(enum CMB_STUB_AIF_X state,
			      enum CMB_STUB_AIF_CTRL ctrl);
void mtk_wcn_cmb_stub_audio_ctrl(enum CMB_STUB_AIF_X state);
void mtk_wcn_cmb_stub_func_ctrl(unsigned int type, unsigned int on);
int mtk_wcn_cmb_stub_do_reset(unsigned int type);
int mtk_wcn_cmb_stub_query_ctrl(void);
int mtk_wcn_cmb_stub_trigger_assert(void);
void mtk_wcn_cmb_stub_clock_fail_dump(void);

/* defined in common_detect/mtk_wcn_stub_alps.c */
int mtk_wcn_wmt_chipid_query(void);
void mtk_wcn_wmt_set_chipid(int chipid);



#endif /* _MTK_WCN_CMB_STUB_H_ */
