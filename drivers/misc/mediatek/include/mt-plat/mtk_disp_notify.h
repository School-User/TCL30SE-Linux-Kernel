/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Shim for MediaTek's mtk_disp_notify.h.  The vendor kernel reported display
 * blank/unblank events to CONNSYS so it could enter deep idle.  Mainline has no
 * equivalent; power saving is simply not wired, which is harmless.
 */
#ifndef _MTK_DISP_NOTIFY_H_
#define _MTK_DISP_NOTIFY_H_

#include <linux/notifier.h>

/* Values follow the vendor DISP notifier ABI; nothing raises them here. */
#define MTK_DISP_EVENT_BLANK 0x01
#define MTK_DISP_BLANK_UNBLANK 0x00
#define MTK_DISP_BLANK_POWERDOWN 0x01

static inline int mtk_disp_notifier_register(const char *name,
					    struct notifier_block *nb)
{
	return 0;
}

static inline int mtk_disp_notifier_unregister(struct notifier_block *nb)
{
	return 0;
}

#endif /* _MTK_DISP_NOTIFY_H_ */
