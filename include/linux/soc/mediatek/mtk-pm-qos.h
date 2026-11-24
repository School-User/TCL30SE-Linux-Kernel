/* SPDX-License-Identifier: GPL-2.0 */
/* Shim: MediaTek's PM-QoS wrapper; mainline uses <linux/pm_qos.h> directly. */
#ifndef _MTK_PM_QOS_H_
#define _MTK_PM_QOS_H_

#include <linux/pm_qos.h>

struct mtk_pm_qos_request {
	struct pm_qos_request req;
};

static inline void mtk_pm_qos_add_request(struct mtk_pm_qos_request *req,
					  int pm_qos_class, s32 value) { }
static inline void mtk_pm_qos_remove_request(struct mtk_pm_qos_request *req) { }
static inline void mtk_pm_qos_update_request(struct mtk_pm_qos_request *req,
					     s32 value) { }
static inline s32 mtk_pm_qos_read_value(int pm_qos_class) { return 0; }

/* DDR OPP requests (vendor pm-qos classes) */
enum mtk_pm_qos_class {
	MTK_PM_QOS_DDR_OPP = 0,
};

#define DDR_OPP_0	0
#define DDR_OPP_UNREQ	(-1)

#endif /* _MTK_PM_QOS_H_ */
