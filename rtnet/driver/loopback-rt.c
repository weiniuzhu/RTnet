/* loopback.c
 *
 * Copyright (C) 2002 Ulrich Marx <marx@kammer.uni-hannover.de>
 * extended by Jose Carlos Billalabeitia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/netdevice.h>

#include <crc32.h>
#include <rtnet.h>
#include <rtnet_internal.h>
#include <rtnet_port.h>

//#define DEBUG_LOOPBACK_DRIVER

static int rt_loopback_in_use;

/***
 *	rt_loopback_open 
 *	@rtdev
 */
static int rt_loopback_open (struct rtnet_device *rtdev) 
{
	if( !rt_loopback_in_use )
	{
		rt_loopback_in_use = 1;
		rt_stack_connect(rtdev, &STACK_manager);
		rtnetif_start_queue(rtdev);
		return 1;
	}

	return 0;
}


/***
 *	rt_loopback_close 
 *	@rtdev
 */
static int rt_loopback_close (struct rtnet_device *rtdev) 
{
	rt_loopback_in_use = 0;
	rtnetif_stop_queue(rtdev);
	rt_stack_disconnect(rtdev);
	
	return 0;
}


/***
 * rt_loopback_xmit - begin packet transmission
 * @skb: packet to be sent
 * @dev: network device to which packet is sent
 *
 */
static int rt_loopback_xmit(struct rtskb *skb, struct rtnet_device *rtdev)
{
	int err=0;
	struct rtskb *new_skb;
	struct net_device *dev=dev_get_by_rtdev(rtdev);
	
	if ( (new_skb=dev_alloc_rtskb(skb->len + 2))==NULL ) 
	{
		rt_printk("RTnet %s: couldn't allocate a rtskb of size %d.\n", dev->name, skb->len);
		err = -ENOMEM;
		goto rt_loopback_xmit_end;
	}
	else 
	{
		new_skb->rx = rt_get_time();
		new_skb->rtdev = rtdev;
		rtskb_reserve(new_skb,2);
		memcpy(new_skb->buf_start, skb->buf_start, SKB_DATA_ALIGN(ETH_FRAME_LEN));
		rtskb_put(new_skb, skb->len);
		new_skb->protocol = rt_eth_type_trans(new_skb, rtdev);

#ifdef DEBUG_LOOPBACK_DRIVER
		{
			int i, cuantos;
			rt_printk("\n\nPACKET:");
			rt_printk("\nskb->protocol = %d", 		skb->protocol);
			rt_printk("\nskb->pkt_type = %d", 		skb->pkt_type);
			rt_printk("\nskb->users = %d", 			skb->users);
			rt_printk("\nskb->cloned = %d", 		skb->cloned);
			rt_printk("\nskb->csum = %d",	 		skb->csum);
			rt_printk("\nskb->len = %d", 			skb->len);
			
			rt_printk("\nnew_skb->protocol = %d", 	new_skb->protocol);
			rt_printk("\nnew_skb->pkt_type = %d", 	new_skb->pkt_type);
			rt_printk("\nnew_skb->users = %d", 		new_skb->users);
			rt_printk("\nnew_skb->cloned = %d", 	new_skb->cloned);
			rt_printk("\nnew_skb->csum = %d",	 	new_skb->csum);
			rt_printk("\nnew_skb->len = %d", 		new_skb->len);
			
			rt_printk("\n\nETHERNET HEADER:");
			rt_printk("\nMAC dest: "); for(i=0;i<6;i++){ rt_printk("0x%02X ", new_skb->buf_start[i+2]); }
			rt_printk("\nMAC orig: "); for(i=0;i<6;i++){ rt_printk("0x%02X ", new_skb->buf_start[i+8]); }
			rt_printk("\nPROTOCOL: "); for(i=0;i<2;i++){ rt_printk("0x%02X ", new_skb->buf_start[i+14]); }
		
			rt_printk("\n\nIP HEADER:");
			rt_printk("\nVERSIZE : "); for(i=0;i<1;i++){ rt_printk("0x%02X ", new_skb->buf_start[i+16]); }
			rt_printk("\nPRIORITY: "); for(i=0;i<1;i++){ rt_printk("0x%02X ", new_skb->buf_start[i+17]); }
			rt_printk("\nLENGTH  : "); for(i=0;i<2;i++){ rt_printk("0x%02X ", new_skb->buf_start[i+18]); }
			rt_printk("\nIDENT   : "); for(i=0;i<2;i++){ rt_printk("0x%02X ", new_skb->buf_start[i+20]); }
			rt_printk("\nFRAGMENT: "); for(i=0;i<2;i++){ rt_printk("0x%02X ", new_skb->buf_start[i+22]); }
			rt_printk("\nTTL     : "); for(i=0;i<1;i++){ rt_printk("0x%02X ", new_skb->buf_start[i+24]); }
			rt_printk("\nPROTOCOL: "); for(i=0;i<1;i++){ rt_printk("0x%02X ", new_skb->buf_start[i+25]); }
			rt_printk("\nCHECKSUM: "); for(i=0;i<2;i++){ rt_printk("0x%02X ", new_skb->buf_start[i+26]); }
			rt_printk("\nIP ORIGE: "); for(i=0;i<4;i++){ rt_printk("0x%02X ", new_skb->buf_start[i+28]); }
			rt_printk("\nIP DESTI: "); for(i=0;i<4;i++){ rt_printk("0x%02X ", new_skb->buf_start[i+32]); }
		
			cuantos = (int)(*(unsigned short *)(new_skb->buf_start+18)) - 20;
			rt_printk("\n\nDATA (%d):", cuantos);  
			rt_printk("\n:");  		   for(i=0;i<cuantos;i++){ rt_printk("0x%02X ", new_skb->buf_start[i+36]); }		
		}
#endif

		rtnetif_rx(new_skb);
		rt_mark_stack_mgr(rtdev);
		dev->last_rx = jiffies;
	}
	
rt_loopback_xmit_end:
	kfree_rtskb(skb);
	return err;
}


/***
 *	loopback_init
 */
static int __init loopback_init(void) 
{
	int err=0;
	struct net_device *dev=&loopback_dev;
	struct rtnet_device *rtdev;

	rt_printk("initializing loopback...\n");
	
	if ( (rtdev=rtdev_alloc(dev))==NULL )
		{ return -ENODEV; }

	rtdev->open = &rt_loopback_open;
	rtdev->stop = &rt_loopback_close;
	rtdev->hard_header = rt_eth_header;
	rtdev->hard_start_xmit = &rt_loopback_xmit;

	rt_loopback_in_use = 0;
	
	return err;
}


/***
 *	loopback_cleanup
 */
static void __exit loopback_cleanup(void) 
{
	struct net_device *dev = &loopback_dev;
	struct rtnet_device *rtdev = rtdev_get_by_dev(dev);

	rt_printk("removing loopback...\n");
	if ( !dev || !rtdev ) 
		{ rt_printk("no loopback device\n"); }

	kfree(rtdev);
}

module_init(loopback_init);
module_exit(loopback_cleanup);
