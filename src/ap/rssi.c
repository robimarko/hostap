/*
 * RSSI control configuration
 * Copyright (c) 2019, Robert Marko <robimarko@gmail.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "utils/includes.h"
#include "utils/common.h"
#include "utils/eloop.h"
#include "utils/wpabuf.h"
#include "common/ieee802_11_defs.h"
#include "hostapd.h"
#include "neighbor_db.h"
#include "wps_hostapd.h"
#include "sta_info.h"
#include "ap_drv_ops.h"
#include "beacon.h"
#include "rrm.h"
#include "wnm_ap.h"
#include "taxonomy.h"
#include "rssi.h"

static void hostapd_bss_signal_check(void *eloop_data, void *user_ctx)
/* This is called by an eloop timeout.  All stations in the list are checked
 * for signal level.  This requires calling the driver, since hostapd doesn't
 * see packets from a station once it is fully authorized.
 * Stations with signal level below the threshold will be dropped.
 * Cases where the last RSSI is significantly less than the average are usually
 * a bad reading and should not lead to a drop.
 */
{
	struct hostapd_data *hapd = user_ctx;
	struct hostap_sta_driver_data data;
	struct sta_info *sta, *sta_next;
	u8 addr[ETH_ALEN];  // Buffer the address for logging purposes, in case it is destroyed while dropping
	int strikes;        //    same with strike count on this station.
	int num_sta = 0;
	int num_drop = 0;
	int signal_inst;
	int signal_avg;


	for (sta = hapd->sta_list; sta; sta = sta_next) {
		sta_next = sta->next;
		memcpy(addr, sta->addr, ETH_ALEN);
		if (!hostapd_drv_read_sta_data(hapd, &data, addr)) { 
			signal_inst = data.signal;
			signal_avg = data.last_ack_rssi;
			num_sta++;
			strikes = sta->sig_drop_strikes;
			if (signal_inst > signal_avg) 
				signal_avg = signal_inst;
			if (signal_inst > (signal_avg - 5)) {  // ignore unusually low instantaneous signal.
				if (signal_avg < hapd->conf->signal_stay_min) { // signal bad.
					strikes = ++sta->sig_drop_strikes;
					if (strikes >= hapd->conf->signal_strikes) {  // Struck out--, drop.
						ap_sta_deauthenticate(hapd, sta, hapd->conf->signal_drop_reason); 
						num_drop++;
					}
				}
				else {
					sta->sig_drop_strikes = 0;  // signal OK, reset the strike counter.
					strikes = 0;
				}				
			}
			hostapd_logger(hapd, addr, HOSTAPD_MODULE_IAPP, HOSTAPD_LEVEL_DEBUG, "%i %i (%i)",
				data.signal, data.last_ack_rssi, strikes);
		}
	}
	hostapd_logger(hapd, NULL, HOSTAPD_MODULE_IAPP, HOSTAPD_LEVEL_INFO, "signal poll: %i STAs, %i dropped", num_sta, num_drop);

	eloop_register_timeout(hapd->conf->signal_poll_time, 0, hostapd_bss_signal_check, eloop_data, hapd);
}
