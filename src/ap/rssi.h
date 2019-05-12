/*
 * RSSI control configuration
 * Copyright (c) 2019, Robert Marko <robimarko@gmail.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef RSSI_CONTROL_H
#define RSSI_CONTROL_H

#ifdef CONFIG_RSSI_CONTROL

static void hostapd_bss_signal_check(void *eloop_data, void *user_ctx);
int hostapd_bss_signal_policy_init(struct hostapd_iface *iface);

#else /* CONFIG_RSSI_CONTROL */

static inline void hostapd_bss_signal_check(void *eloop_data, void *user_ctx)
{

}

static inline int hostapd_bss_signal_policy_init(struct hostapd_iface *iface)
{
	return -1;
}

#endif /* CONFIG_RSSI_CONTROL */

#endif /* RSSI_CONTROL_H */