/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Compatibility shim for the legacy <linux/of_gpio.h> API removed in 6.8.
 * Only used by the out-of-tree MediaTek connectivity port.
 *
 * The legacy API returned a global GPIO number; mainline is descriptor-only,
 * so these helpers are not equivalent.  Callers in the connectivity tree must
 * be converted to <linux/gpio/consumer.h>; until then they fail cleanly.
 */
#ifndef _LINUX_OF_GPIO_COMPAT_H
#define _LINUX_OF_GPIO_COMPAT_H

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>

enum of_gpio_flags {
	OF_GPIO_ACTIVE_LOW = 0x1,
	OF_GPIO_SINGLE_ENDED = 0x2,
	OF_GPIO_OPEN_DRAIN = 0x4,
	OF_GPIO_TRANSITORY = 0x8,
	OF_GPIO_PULL_UP = 0x10,
	OF_GPIO_PULL_DOWN = 0x20,
};

static inline int of_gpio_named_count(const struct device_node *np,
				      const char *propname)
{
	return of_count_phandle_with_args(np, propname, "#gpio-cells");
}

static inline int of_gpio_count(const struct device_node *np)
{
	return of_gpio_named_count(np, "gpios");
}

static inline int of_get_named_gpio_flags(const struct device_node *np,
					  const char *propname, int index,
					  enum of_gpio_flags *flags)
{
	/* TODO(port): convert callers to gpiod; see HANDOFF "Wi-Fi port". */
	return -ENOSYS;
}

static inline int of_get_named_gpio(const struct device_node *np,
				    const char *propname, int index)
{
	return of_get_named_gpio_flags(np, propname, index, NULL);
}

#endif /* _LINUX_OF_GPIO_COMPAT_H */
