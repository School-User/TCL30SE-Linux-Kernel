/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Types removed from mainline since 4.19 that the MediaTek connectivity tree
 * still uses.  Kept in one place so the port surface is greppable.
 */
#ifndef _MTK_CONNECTIVITY_COMPAT_H_
#define _MTK_CONNECTIVITY_COMPAT_H_

#include <linux/types.h>
#include <linux/pm.h>
#include <linux/device.h>

/* 4.19 legacy PM callback (removed: no users left in mainline) */
typedef int (*pm_callback_t)(pm_message_t state, void *data);

/* MediaTek msdc SDIO glue (msdc_sdio_irq_handler_t / struct sdio_ops) */
typedef void (*msdc_sdio_irq_handler_t)(void *data);

struct sdio_ops {
	void (*sdio_request_eirq)(msdc_sdio_irq_handler_t irq_handler,
				  void *data);
	void (*sdio_enable_eirq)(void);
	void (*sdio_disable_eirq)(void);
	void (*sdio_register_pm)(pm_callback_t pm_cb, void *data);
};

/* Interface identifiers used by the combo deep-idle helpers */
enum COMBO_IF {
	COMBO_IF_UART = 0,
	COMBO_IF_MSDC,
	COMBO_IF_BTIF,
	COMBO_IF_MAX,
};

/* Implemented in common/common_detect/mtk_wcn_stub_alps.c.  The vendor kernel
 * declared these in mt_combo_plt.h, which is not part of this import.
 */
int mt_combo_plt_enter_deep_idle(enum COMBO_IF src);
int mt_combo_plt_exit_deep_idle(enum COMBO_IF src);

#endif /* _MTK_CONNECTIVITY_COMPAT_H_ */

