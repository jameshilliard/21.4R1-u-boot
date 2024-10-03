/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

#ifdef CONFIG_POST

#include <configs/ex3242.h>
#include "syspld.h"
#include <command.h>
#include <post.h>

#if CONFIG_POST & CFG_POST_SYSPLD
extern uint bd_id;
extern int i2c_res;
extern int board_id(void);
extern  int lcd_gone;
static int syspld_debug_flag;

int syspld_post_test (int flags)
{
	int ret = 0;
	int pass = 1;
	int i = 0;
	int total_result = 0;
	int syspld_result = 0;
	int count_syspld_flag[200][2][2][4] ={{{{0}}}};
	int k = 0;
	int port_num = 0;
	int ports_test_count = 0;
	int max_port = 0;
	int pt_init_value = 0;
	syspld_debug_flag = 0;
	PortLink link;
	PortDuplex duplex;
	PortSpeed speed;
	uint java_bd_id = 0;

	if (flags & POST_DEBUG) {
		syspld_debug_flag = 1;
	}

	if (bd_id == 0) {
		java_bd_id = board_id();
	}
	else {
		java_bd_id = bd_id;
	}

	switch (java_bd_id) {
	  case I2C_ID_JUNIPER_SUMATRA:
		  max_port = 48;		  
		  if (syspld_debug_flag)
		  post_log(" This tests checks the functionlity of LCD back light,"
				   " Heart Beat of LCD,Port Led From 0 to 47 \n");
		  break;
	  case I2C_ID_JUNIPER_EX3200_24_T:
		  max_port = 24;		  
		  if (syspld_debug_flag)
		  post_log(" This tests checks the functionlity of LCD back light,"
				   " Heart Beat of LCD,Port Led From 0 to 23 \n");
		  break;
	  case I2C_ID_JUNIPER_EX3200_24_P:
		  max_port = 24;		  
		  if (syspld_debug_flag)
		  post_log(" This tests checks the functionlity of LCD back light,"
				   " Heart Beat of LCD,Port Led From 0 to 23 \n");
		  break;
	  case I2C_ID_JUNIPER_EX3200_48_T:
		  max_port = 48;		  
		  if (syspld_debug_flag)
		  post_log(" This tests checks the functionlity of LCD back light,"
				   " Heart Beat of LCD,Port Led From 0 to 47 \n");
		  break;
	  case I2C_ID_JUNIPER_EX3200_48_P:
		  max_port = 48;		  
		  if (syspld_debug_flag)
		  post_log(" This tests checks the functionlity of LCD back light,"
				   " Heart Beat of LCD,Port Led From 0 to 47 \n");
		  break;
	  case I2C_ID_JUNIPER_EX4200_24_F:
		  max_port = 24;		  
		  if (syspld_debug_flag)
		  post_log(" This tests checks the functionlity of LCD back light,"
				   " Heart Beat of LCD,Port Led From 0 to 23 \n");
		  break;
	  case I2C_ID_JUNIPER_EX4200_24_T:
		  max_port = 24;		  
		  if (syspld_debug_flag)
		  post_log(" This tests checks the functionlity of LCD back light,"
				   " Heart Beat of LCD,Port Led From 0 to 23 \n");
		  break;
	  case I2C_ID_JUNIPER_EX4200_24_P:
		  max_port = 24;		  
		  if (syspld_debug_flag)
		  post_log(" This tests checks the functionlity of LCD back light,"
				   " Heart Beat of LCD,Port Led From 0 to 23 \n");
		  break;
	  case I2C_ID_JUNIPER_EX4200_48_T:
		  max_port = 48;		  
		  if (syspld_debug_flag)
		  post_log(" This tests checks the functionlity of LCD back light,"
				   " Heart Beat of LCD,Port Led From 0 to 47 \n");
		  break;
	  case I2C_ID_JUNIPER_EX4200_48_P:
		  max_port = 48;		  
		  if (syspld_debug_flag)
		  post_log(" This tests checks the functionlity of LCD back light,"
				   " Heart Beat of LCD,Port Led From 0 to 47 \n");
		  break;
	  default:
		  max_port = 48;		  
		  if (syspld_debug_flag)
		  post_log(" This tests checks the functionlity of LCD back light,"
				   " Heart Beat of LCD,Port Led From 0 to 47 \n");
		  break;
	};

	if (!lcd_gone) {
		if (syspld_debug_flag) {
			post_log(" **************  Testing LCD *******\n"); 
		}

		lcd_init();
		ret = lcd_backlight_control(LCD_BACKLIGHT_OFF);
		if ( ret != pass ) {
			syspld_result++;
			count_syspld_flag[k++][0][0][0] =1;
		}	
		else {
			udelay(500000);
		}

		ret = lcd_backlight_control(LCD_BACKLIGHT_ON);
		if ( ret != pass ) {
			syspld_result++;
			count_syspld_flag[k++][0][0][0] =1;
		}	

		ret = lcd_heartbeat();
		if (ret != pass) {
			syspld_result++;
			count_syspld_flag[k++][0][0][0] =1;
		}
		else {
			udelay(500000);
		}
	}

	//*******  Testing the state of port led from 1 to 48 *******
	if (syspld_debug_flag) {
		post_log("\n **************  Testing Port  Led for LinkUp and LinkDown  *******\n"); 
	}
	led_init();
	for (i = 1; i <= max_port; i++) {
		for (link = LINK_DOWN; link <= LINK_UP ; link++) {
			for (duplex =  HALF_DUPLEX; duplex <= FULL_DUPLEX ; duplex++) {
				for (speed = SPEED_10M ; speed <= SPEED_10G ; speed++) {
					ret = set_port_led(i, link, duplex, speed);
					if (ret != pass) {
						syspld_result++;
						if (syspld_debug_flag)
						post_log(".");
						count_syspld_flag[k][link][duplex][speed] =1;
					}
					else {
						if (syspld_debug_flag)
						post_log(".");
						udelay(100000);
					}
				}
			}
		}
		k++;
	}

	for (i = 1; i <= max_port; i++) {
		for (link = LINK_DOWN; link == LINK_DOWN ; link++) {
			for (duplex =  HALF_DUPLEX; duplex <= FULL_DUPLEX ; duplex++) {
				for (speed = SPEED_10M ; speed <= SPEED_10G ; speed++) {
					ret = set_port_led(i, link, duplex, speed);
					if (ret != pass) {
						if (syspld_debug_flag)
						post_log(".");
						count_syspld_flag[k][link][duplex][speed] =1;
						syspld_result++;
					}
					else {
						if (syspld_debug_flag)
						post_log(".");
					}
				}
			}
		}
		k++;
	}

	//Displays syspld result 	
	if (syspld_result ==  total_result ) {
		post_log("-- SYSPLD                               PASSED\n");
		if (syspld_debug_flag) {
			if (!lcd_gone) {
				post_log("\n  LcdBackLight OFF functional  \n");
				post_log("\n  LcdBackLight ON functional  \n");
				post_log("\n  Lcd Heart beat test passed  \n");
			}
			else {
			post_log("\n   LCD module not detected   \n");
			}
			for (i = 0; i < max_port; i++) {
				for (link = LINK_DOWN; link <= LINK_UP ; link++) {
					for (duplex =  HALF_DUPLEX; duplex <= FULL_DUPLEX ; duplex++) {
						for (speed = SPEED_10M ; speed <= SPEED_10G ; speed++) {
							post_log("\n  Port led set linkup/down" 
									 ":  link = %s"
									 " duplex = %s speed = %s for the port  %d  \n",
									 link_status[link], duplex_mode[duplex],
									 port_speed[speed], i);
						}
					}
				}
			}
			for (i = 0; i < max_port; i++) {
				for (link = LINK_DOWN; link == LINK_DOWN ; link++) {
					for (duplex =  HALF_DUPLEX; duplex <= FULL_DUPLEX ; duplex++) {
						for (speed = SPEED_10M ; speed <= SPEED_10G ; speed++) {
							post_log(" Port led set linkdown" 
									 ":  link = %s"
									 " duplex = %s speed = %s for the port  %d\n",
									 link_status[link], duplex_mode[duplex],
									 port_speed[speed], i);
						}
					}
				}
			}
		}
		return 0;
	}
	else {
		k = 0;
		post_log("-- SYSPLD                               FAILED @\n");
		if (!lcd_gone) {
			if (count_syspld_flag[k++][0][0][0] ==1) {
				post_log("\n  LcdBackLight OFF not functional  \n");
			}
			else {
				if (syspld_debug_flag) 
					post_log("\n  LcdBackLight OFF functional  \n");
			}
			if (count_syspld_flag[k++][0][0][0] ==1) {
				post_log("\n  LcdBackLight ON not functional  \n");
			}
			else {
				if (syspld_debug_flag) 
					post_log("\n  LcdBackLight ON functional  \n");
			}
			if (count_syspld_flag[k++][0][0][0] ==1) {
				post_log("\n  Lcd Heart beat test failed  \n");
			}
			else {
				if (syspld_debug_flag) 
					post_log("\n  Lcd Heart beat test passed  \n");
			}
			pt_init_value =k;
			if (max_port == 24) {
				ports_test_count = pt_init_value + 24; 
			}
			else {
				ports_test_count = pt_init_value + 48;
			}
		}
		else {
			post_log("\n   LCD module not detected   \n");
			pt_init_value = 0;
			if (max_port == 24) {
				ports_test_count = pt_init_value + 24; 
			}
			else {
				ports_test_count = pt_init_value + 48;
			}
		}

		for (k = pt_init_value; k<ports_test_count; k++) {
			for (link = LINK_DOWN; link <= LINK_UP ; link++) {
				for (duplex =  HALF_DUPLEX; duplex <= FULL_DUPLEX ; duplex++) {
					for (speed = SPEED_10M ; speed <= SPEED_10G ; speed++) {
						if (count_syspld_flag[k][link][duplex][speed] == 1) {
							post_log("\n  Port led failed to set linkup/down" 
									 " :  link = %s"
									 " duplex = %s speed = %s for the port"
									 " number  %d  \n",
									 link_status[i], duplex_mode[duplex],
									 port_speed[speed], port_num);
						}
						else {
							if (syspld_debug_flag) { 
								post_log("\n  Port led set linkup/down" 
										 " :  link = %s"
										 " duplex = %s speed = %s for the port "
										 "number %d  \n",
										 link_status[link], duplex_mode[duplex],
										 port_speed[speed], port_num);
							}
						}
					}
				}
			}
			port_num++;
		}

		port_num = 0;
		if (!lcd_gone) {
			if (max_port == 24) {
				pt_init_value = ports_test_count;
				ports_test_count += 24; 
			}
			else {
				pt_init_value = ports_test_count;
				ports_test_count += 48; 
			}
		}
		else {
			if (max_port == 24) {
				pt_init_value = ports_test_count;
				ports_test_count += 24; 
			}
			else {
				pt_init_value = ports_test_count;
				ports_test_count += 48; 
			}
		}

		for (k = pt_init_value; k<ports_test_count; k++) {
			for (link = LINK_DOWN; link == LINK_DOWN; link++) {
				for (duplex =  HALF_DUPLEX; duplex <= FULL_DUPLEX ; duplex++) {
					for (speed = SPEED_10M ; speed <= SPEED_10G ; speed++) {
						if (count_syspld_flag[k][link][duplex][speed] == 1) {
							post_log("\n  Port led failed to set linkdown" 
									 " :  link = %s"
									 " duplex = %s speed = %s for the port"
									 " number  %d  \n",
									 link_status[i], duplex_mode[duplex],
									 port_speed[speed], port_num);
						}
						else {
							if (syspld_debug_flag) { 
								post_log("\n  Port led set linkdown" 
										 " :  link = %s"
										 " duplex = %s speed = %s for the port "
										 "number %d  \n",
										 link_status[link], duplex_mode[duplex],
										 port_speed[speed], port_num);
							}

						}
					}
				}
			}
			port_num++;
		}
		return -1;
	}
}

#endif /* CONFIG_POST & CFG_POST_SYSPLD */
#endif /* CONFIG_POST */
