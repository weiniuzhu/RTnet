/***
 *
 *  rtcfg/rtcfg_timer.c
 *
 *  Real-Time Configuration Distribution Protocol
 *
 *  Copyright (C) 2003, 2004 Jan Kiszka <jan.kiszka@web.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/list.h>

#include <rtdev.h>
#include <rtcfg/rtcfg.h>
#include <rtcfg/rtcfg_conn_event.h>
#include <rtcfg/rtcfg_event.h>
#include <rtcfg/rtcfg_frame.h>


void rtcfg_timer(int ifindex)
{
    struct rtcfg_device     *rtcfg_dev = &device[ifindex];
    struct list_head        *entry;
    struct rtcfg_connection *conn;
    int                     last_stage_1 = -1;
    int                     burst_credit;
    int                     index;
    int                     ret;


    while (1) {
        rtos_res_lock(&rtcfg_dev->dev_lock);

        if (rtcfg_dev->state == RTCFG_MAIN_SERVER_RUNNING) {
            index = 0;
            burst_credit = rtcfg_dev->burstrate;

            list_for_each(entry, &rtcfg_dev->conn_list) {
                conn = list_entry(entry, struct rtcfg_connection, entry);

                if (conn->state == RTCFG_CONN_SEARCHING) {
                    if ((burst_credit > 0) && (index > last_stage_1)) {
                        if ((ret = rtcfg_send_stage_1(conn)) < 0) {
                            RTCFG_DEBUG(2, "RTcfg: error %d while sending "
                                        "stage 1 frame\n", ret);
                        }
                        burst_credit--;
                        last_stage_1 = index;
                    }
                } else {
                    /* skip connection which do not send stage 1 frames */
                    if (last_stage_1 == (index-1))
                        last_stage_1 = index;

                    rtcfg_do_conn_event(conn, RTCFG_TIMER, NULL);
                }
                index++;
            }

            /* handle pointer overrun of the last stage 1 transmission */
            if (last_stage_1 == (index-1))
                last_stage_1 = -1;
        }
        /* TODO:
        else if (rtcfg_dev->state == RTCFG_MAIN_CLIENT_2)
            rtcfg_send_heartbeat(ifindex);*/

        rtos_res_unlock(&rtcfg_dev->dev_lock);

        rtos_task_wait_period();
    }
}
