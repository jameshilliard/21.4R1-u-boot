/*
 * Copyright (c) 2008-2013, Juniper Networks, Inc.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "rps.h"
#include <exports.h>
#include "rps_command.h"
#include "rps_cpld.h"
#include "rps_mbox.h"
#include "rps_psMon.h"
#include "rps_portMon.h"
#include "rps_etq.h"
#include "rps_msg.h"
#include "rps_led.h"
#include "rps_priority.h"

#include <common.h>
extern char rps_sn[];
extern char rps_version_string[];
extern char uboot_version_string[];

#define CMD_MSG 1
#define REQ_MSG 2
#define REPLY_MSG 3

#define MSG_TYPE 0
#define ECHO_BYTE 1
#define MSG_BYTE 4
#define DATA_START_BYTE 5

#define EX3200_24_T           36  /* Java Espresso */
#define EX3200_24_P           37  /* Java Espresso POE */
#define EX3200_48_T           38  /* Java Espresso */
#define EX3200_48_P           39  /* Java Espresso POE */
#define EX4200_24_F           40  /* Java Latte SFP */
#define EX4200_24_T           41  /* Java Latte */
#define EX4200_24_P           42  /* Java Latte POE */
#define EX4200_48_T           43  /* Java Latte */
#define EX4200_48_P           44  /* Java Latte POE */

#define EX2200_24_T           75  /* Jasmine 24  */
#define EX2200_24_P           76  /* Jasmine 24 POE */
#define EX2200_48_T           77  /* Jasmine 48 */
#define EX2200_48_P           78  /* Jasmine 48 POE */

#define EX3300_24_T           112  /* Dragon  24  */
#define EX3300_24_P           113  /* Dragon 24 POE */
#define EX3300_48_T           114  /* Dragon 48 */
#define EX3300_48_P           115  /* Dragon  48 POE */
#define EX3300_24_DC          116  /* Dragon  24  */
#define EX3300_48_BF          117  /* Dragon  48 POE */

#define EX_12V_PWR           190  /* Java 12V PSU Power */
#define EX_48V_NONPOE_PWR    130  /* Java 48V PSU NON Poe Power */  
#define EX_48V_24_POE_PWR    410  /* Java 48V 24 Port PSU Poe Power */ 
#define EX_48V_48_POE_PWR    740  /* Java 48V 48 Port PSU Poe Power */ 
 
#define PSU_EMPTY              0  /* PSU empty */
#define PSU_OFFLINE            1  /* PSU present and bad */
#define PSU_ONLINE             2  /* PSU present and good */

#define MAX_VER_LEN            4  /* Firmware version length */
#define MAX_IP_LEN             16 /* IP address max len */

typedef enum msgCmd 
{
    RPS_LED = 1,
    RPS_PSU,
    RPS_STATUS,
    RPS_INFO,
    RPS_PRIORITY,
    RPS_DISABLE_BKUP,
    RPS_UPGRADE,
    RPS_UPG_STAT,
    RPS_SCALE,
    RPS_SHOW_SCALE,
    RPS_NW_PARAMS,
    RPS_SHOW_NW,
} MsgCmd;


typedef struct rps_info {
    uint32_t          fpc_slot;
    uint8_t           rps_serial_no[13];     /* rps serial number */
    uint8_t           rps_model;          /* index to table of rps model no. */
    uint8_t           rps_firmware_ver[12];  /* rps firmware version */
    uint8_t           uboot_ver[12];         /* rps u-boot version */
    uint8_t           port_no;      /* rps port to which switch is connected */
    uint8_t           backup_status;   /* whether switch is backed-up or not */
} rps_info_t;

typedef struct rps_show_led_cmd {
    uint32_t          fpc_slot;
    uint8_t           rps_fan_status_index;
    uint8_t           rps_system_status_index;
    uint8_t           port_no[CPLD_MAX_PORT];
    uint8_t           rps_port_led_index[CPLD_MAX_PORT];
} rps_show_led_cmd_t;

typedef struct rps_show_psu_cmd {
    uint32_t          fpc_slot;
    uint8_t           psu_slot_id[CPLD_MAX_PS];
    uint8_t           status[CPLD_MAX_PS];
    uint8_t           power_rating[CPLD_MAX_PS];
} rps_show_psu_cmd_t;

typedef struct rps_show_status_cmd {
    uint32_t    fpc_slot;
    uint8_t     port_no[CPLD_MAX_PORT]; /* rps port to switch */
    uint8_t     backup_status_index[CPLD_MAX_PORT];  /* index back-up status */
    uint32_t    priority[CPLD_MAX_PORT];              /* rps switch priority */
    uint32_t    power_requested[CPLD_MAX_PORT]; /* back-up power requested */
    char        sn[CPLD_MAX_PORT][MAX_SN_SZ];   /* Switch serial number */
} rps_show_status_cmd_t;

typedef struct rps_show_upg_cmd {
    uint32_t    fpc_slot;
    uint8_t     upg_stat;  /* 1/0- upgrade pass/fail */
    char        prev_ver[MAX_VER_LEN];      /* prev firmware version */
    char        cur_ver[MAX_VER_LEN];       /* cur firmware version */
} rps_show_upg_cmd_t;

typedef struct rps_show_scale_cmd {
    uint32_t    fpc_slot;
    uint8_t     scale;                       /* 1 /2 for multi-backup no/yes */
} rps_show_scale_cmd_t;

typedef struct rps_show_nw_cmd {
    uint32_t    fpc_slot;
    char        ipaddr[MAX_IP_LEN];          /* ip address */ 
    char        netmask[MAX_IP_LEN];         /* netmask */ 
    char        gateway[MAX_IP_LEN];         /* gateway */ 
} rps_show_nw_cmd_t;

MsgState msg_state[CPLD_MAX_PORT];
static int32_t gMsgTid[CPLD_MAX_PORT];
static int loopCount[CPLD_MAX_PORT];
static uint8_t cmd[CPLD_MAX_PORT], echo[CPLD_MAX_PORT];
char *switchSN[CPLD_MAX_PORT][MAX_SN_SZ];

extern uint32_t gTicksPerMS;
int gwaitms = 1200;
int gDebugMsg = 0;
    
void msgSM(int port, int state);
void sendMboxMessage(int busReq, int busGnt, int port);
void msgHandler(ptData ptdata);
int32_t msgProcessor(int port, uint8_t *data, uint32_t len,
                     uint8_t *cmd, uint8_t *echo);
int32_t buildReply(int port, uint8_t *data, uint32_t* lenReply,
                   uint8_t cmd, uint8_t echo);

extern void upgrade_rps_image(void);
uint8_t 
ledStatusIndex (LedColor color)
{
    uint8_t temp = 0x6;

    switch(color)
    {
        case LED_OFF:
            temp = 6;
            break;
            
        case LED_GREEN_BLINKING:
            temp = 3;
            break;
            
        case LED_GREEN:
            temp = 0;
            break;
            
        case LED_AMBER_BLINKING:
            temp = 5;
            break;
            
        case LED_AMBER:
            temp = 2;
            break;
            
        default:
            temp = 6;
            break;
            
    }

    return (temp);
}


uint8_t 
psIndex (uint16_t psid)
{
    uint8_t temp = 0xff;

    switch(psid)
    {
        case I2C_ID_EX32X42X_PWR_190_DC:
            temp = 3;
            break;
            
        case I2C_ID_EX32X42X_PWR_320:
            temp = 2;
            break;
            
        case I2C_ID_EX32X42X_PWR_600:
            temp = 1;
            break;
            
        case I2C_ID_EX32X42X_PWR_930:
            temp = 0;
            break;
            
        default:
            temp = 0xff;
            break;
            
    }

    return (temp);
}

uint8_t 
powerIndex (int16_t power)
{
    uint8_t temp = 0xff;

    if (power == 930)
        temp = 0;
    else if (power == 600)
        temp = 1;
    else if (power == 320)
        temp = 2;
    else
        temp = 0xff;

    return (temp);
}

void 
initMsg (void)
{
    int i;
    char *s;
    int temp;
    
    if (((s = getenv("rps.debug")) != NULL) &&
         (strcmp(s,"on") == 0))
         gDebugMsg = 1;
    
    if ((s = getenv("rps.wait")) != NULL) {
        temp = simple_strtoul (s, NULL, 10);
        if (temp > 300)
            gwaitms = temp;
    }
    
    for (i = 0; i < CPLD_MAX_PORT; i++) {
        msg_state[i] = POLLING;
        gMsgTid[i] = 0;
    }
}

void 
startMsgMon (int32_t start_time, int32_t cycle_time)
{
    tData tdata;
    int32_t tid;
    int32_t start = start_time / MS_PER_TICK;
    int32_t ticks = cycle_time / MS_PER_TICK;
    uint8_t mbox[2] = {0, 0x40};  /* RPS up */
    int i;

    tdata.param[0] = tdata.param[1] = tdata.param[2] = 0;
    for (i = 0; i < CPLD_MAX_PORT; i++) {
        tdata.param[0] = i;
        msg_state[i] = POLLING;
        set_mbox(i, mbox);
        if  (addTimer(tdata, &tid, PERIDICALLY, start+10*(CPLD_MAX_PORT-i),
                              ticks+CPLD_MAX_PORT-i, msgHandler) == 0)
        {
            gMsgTid[i] = tid;
        }
    }
}

void 
stopMsgMon (void)
{
    int i;
    
    for (i = 0; i < CPLD_MAX_PORT; i++) {
        delTimer(gMsgTid[i]);
        gMsgTid[i] = 0;
    }
}

void 
sendMboxMessage (int busReq, int busGnt, int port)
{
    uint8_t mbox[2], new_mbox[2];
    uint8_t portState[2];
    uint16_t *pMBox, *pNewMBox;
    uint16_t psState;
    int i;

    if (get_mbox(port, mbox))
        return;
        
    if (cpld_read_direct(CPLD_FIRST_STATE_REG, portState, 2))
        return;

    psState = getPSStatus();
    
    pMBox = (uint16_t *)mbox;
    pNewMBox = (uint16_t *)new_mbox;

    *pMBox = swap_ushort(*pMBox);
    *pNewMBox = 0;

    for (i = 0; i < CPLD_MAX_PS; i++) {
        if ((psState & (1<<i*3)) && ((psState & (1<<(i*3+1)))==0))
            *pNewMBox |= 1 << i;
        if ((psState & (1<<i*3)) && (psState & (1<<(i*3+2))))
            *pNewMBox |= 1 << (i+3);
    }
    
    if (*pMBox & 0x4000) {
        if ((*pMBox & 0x3800) == 0x0800) {  /* port state req */
            *pNewMBox |= ((*pMBox & 0x3800) | 
                    ((portState[port/3] & (0x3 << (port%3) * 2))<<
                    (8-((port%3) * 2))));
        } else if ((*pMBox & 0x3800) == 0x1000) {  /* pri req */
            *pNewMBox |= ((*pMBox & 0x3800) | 
                         (getPriorityActualCache(port) & 0x7));
        }
    }
    else {  /* kept mb cmd + reply */
        *pNewMBox |= (*pMBox & 0x3f00);
    }

    if (busReq == 1)
        *pNewMBox |= 0x8000;
    else if (busReq == 0)
        *pNewMBox &= ~0x8000;
    else
        *pNewMBox |= (*pMBox & 0x8000);

    if (busGnt == 1) {
        *pNewMBox |= 0x80;
    }
    else if (busGnt == 0)
        *pNewMBox &= ~0x80;
    else
        *pNewMBox |= (*pMBox & 0x80);

    *pNewMBox |= 0x40;  /* RPS up */
    
    if (busGnt == 1) {
        if (gDebugMsg) {
            printf("busGnt[%d], mbox = %02x %02x/%02x %02x\n", 
                   port,
                   new_mbox[1], new_mbox[0],
                   mbox[1], mbox[0]
                   );
            printf("busGnt tick = %d\n", get_timer(0) / gTicksPerMS);
        }
    }

    *pMBox = swap_ushort(*pMBox);
    *pNewMBox = swap_ushort(*pNewMBox);

    /* all ports.  port by port ? */
    if (*pMBox != *pNewMBox) {
        set_mbox(port, new_mbox);
    }
    
}

void 
msgHandler (ptData ptdata)
{
    int port;
    uint8_t mbox[2];
    int busReq = -1, busGnt = -1;

    port = ptdata->param[0];

    if (port >= CPLD_MAX_PORT)
        return;

    if (!isPortUp(port)) {
        msg_state[port] = POLLING;
        sendMboxMessage(0, 0, port);
        return;
    }

    if (msg_state[port] == POLLING) {
        if (get_mbox(port, mbox))
            return;

        if (mbox[0] & 0x80) {
            msg_state[port] = I2C_CMD;
            busReq = 0;
            busGnt = 1;
        }
        else if (mbox[1] & 0x80) {
            /* in case we got into odd situation, clear bus grant */
            sendMboxMessage(busReq, 0, port);
        }
    }
    else if (msg_state[port] == I2C_REQ) {
        if (get_mbox(port, mbox)) {
            return;
        }
        
        if (mbox[0] & 0x80) {
            msg_state[port] = I2C_REPLY;
            busReq = 0;
            busGnt = 1;
        }
        else if (mbox[1] & 0x80) {
            /* 
             * In case we got into an odd situation, clear bus grant 
             */
            sendMboxMessage(busReq, 0, port);
        }
    }
    sendMboxMessage(busReq, busGnt, port);

    if (msg_state[port] != POLLING) {
        msgSM(port, msg_state[port]);
    }
}

int32_t 
msgProcessor (int port, uint8_t *data, uint32_t len, 
              uint8_t *cmd, uint8_t *echo)
{
    int32_t result = 0;
    uint8_t pri;
    int16_t pwr_12V = 0;
    int16_t pwr_48V = 0;
    int32_t tid;
    uint8_t val;
    tData tdata;
    char temp_str[255];
    uint16_t ip_len, img_len;

    if ((data[MSG_TYPE] < CMD_MSG) || (data[MSG_TYPE] > REPLY_MSG))
        return (-1);

    *cmd = data[MSG_BYTE];
    *echo = data[ECHO_BYTE];

    switch(data[MSG_BYTE])
    {
        case RPS_INFO:
        case RPS_LED:
        case RPS_PSU:
        case RPS_STATUS:
        case RPS_UPG_STAT:
        case RPS_SHOW_SCALE:
        case RPS_SHOW_NW:
            result = REQ_MSG;
            break;

        case RPS_PRIORITY:
            pri = data[DATA_START_BYTE];
            pwr_12V = EX_12V_PWR;

            cpld_reg_read(CPLD_52V_CFG,&val); 
            /* need to determine the pwr requirement based on the board type */
            switch(data[6]) 
            {
                case EX3200_24_T:
                case EX3200_48_T:
                case EX4200_24_T:
                case EX4200_24_F:
                case EX4200_48_T:
                case EX2200_24_T:
                case EX2200_48_T:
                case EX3300_24_T:
                case EX3300_48_T:
                case EX3300_24_DC:
                case EX3300_48_BF:
                    val |= (1 << port);
                    pwr_48V = EX_48V_NONPOE_PWR;
                    break;

                case EX3200_24_P:
                case EX4200_24_P:
                case EX2200_24_P:
                case EX3300_24_P:
                    val &=  ~(1 << port);
                    pwr_48V = EX_48V_24_POE_PWR;
                    break;

                case EX3200_48_P:
                case EX4200_48_P:
                case EX2200_48_P:
                case EX3300_48_P:
                    val &=  ~(1 << port);
                    pwr_48V = EX_48V_48_POE_PWR;
                    break;

                default:
                    break;

            }
            /*
             * Let CPLD know whether to check 52V or not 
             * */
            cpld_reg_write(CPLD_52V_CFG,val);
            /*
             * Always transition to priority 0 before assigning new priority.
             * For the CPLD to change states from oversubscribed/backedup to
             * armed, priority should first be made 0 and then non-zero.
             */
            setPriority(port, 0);
            udelay(1000); 
            priorityControl(port, data[DATA_START_BYTE], pwr_12V, pwr_48V);

            strncpy(switchSN[port], &data[DATA_START_BYTE + 2], MAX_SN_SZ);
            result = CMD_MSG;
            break;

	case RPS_DISABLE_BKUP:

	    pri = getPriorityRequested(port);
	    pwr_12V = get12VPowerRequested(port);
	    pwr_48V = get48VPowerRequested(port);
	    tdata.param[0] = port;
	    tdata.param[1] = pri;
	    tdata.param[2] = (pwr_12V << 16) + pwr_48V;
	    addTimer(tdata, &tid, ONE_SHOT, 500, 0, pri_Handler);
	    break;
        
    case RPS_UPGRADE:
        /* 
         * 1st data byte is server ip length 
         * followed by server ip, image path len and image path name 
         */
        ip_len = data[DATA_START_BYTE];
        strncpy(temp_str, &data[DATA_START_BYTE + 1], ip_len);
        temp_str[ip_len] = '\0';
        setenv("serverip", temp_str);

        img_len = data[DATA_START_BYTE + ip_len + 1];
        strncpy(temp_str, &data[DATA_START_BYTE + ip_len + 2], img_len);
        temp_str[img_len] = '\0';
        setenv("upg_img", temp_str);
        upgrade_rps_image();
        break;

    case RPS_SCALE:
        if (data[DATA_START_BYTE] == SCALE_2) {
            setenv("scale", "yes");
            setMaxActive(SCALE_2);
        }
        else {
            setenv("scale", "no");
            setMaxActive(SCALE_1);
        }
        saveenv();
        break;

    case RPS_NW_PARAMS:
        /*
         * Get ip address, netmask and gateway
         */
        ip_len = strlen(&data[DATA_START_BYTE]);
        strncpy(temp_str, &data[DATA_START_BYTE], ip_len);
        temp_str[ip_len] = '\0';
        setenv("ipaddr", temp_str);

        ip_len = strlen(&data[DATA_START_BYTE + MAX_IP_LEN]);
        strncpy(temp_str, &data[DATA_START_BYTE + MAX_IP_LEN], ip_len);
        temp_str[ip_len] = '\0';
        setenv("netmask", temp_str);

        ip_len = strlen(&data[DATA_START_BYTE + 2 * MAX_IP_LEN]);
        strncpy(temp_str, &data[DATA_START_BYTE + 2 * MAX_IP_LEN], ip_len);
        temp_str[ip_len] = '\0';
        setenv("gatewayip", temp_str);
        saveenv();
        break;

    }    
    return (result);
}

int32_t 
buildReply (int port, uint8_t *data, uint32_t* lenReply, 
            uint8_t cmd, uint8_t echo)
{
    uint16_t  msg_len = 0;
    uint8_t   buff[256];
    int32_t   result = 0;
    int32_t   i  = 0;
    uint16_t  psState;
    uint8_t   psu_present;
    uint8_t   psu_pwr_good;

    rps_info_t            *rps_info_msg; 
    rps_show_led_cmd_t    *rps_led_msg; 
    rps_show_psu_cmd_t    *rps_psu_msg; 
    rps_show_status_cmd_t *rps_status_msg; 
    rps_show_upg_cmd_t    *rps_upg_msg;
    rps_show_scale_cmd_t  *rps_scale_msg;
    rps_show_nw_cmd_t     *rps_nw_msg;

    data[MSG_TYPE] = REPLY_MSG; 
    data[ECHO_BYTE] = echo; 
    data[MSG_BYTE] = cmd; 

    memset(buff, 0x0, sizeof(buff));

    switch(cmd)
    {
        case RPS_INFO:
            msg_len = sizeof(rps_info_t);
            rps_info_msg = (rps_info_t *)buff;

            memcpy(rps_info_msg->rps_firmware_ver, rps_version_string, 3);
            rps_info_msg->rps_model = 0;
            memcpy(rps_info_msg->rps_serial_no, rps_sn, 12);
            memcpy(rps_info_msg->uboot_ver, uboot_version_string, 5);

            rps_info_msg->port_no = (uint8_t)port;
            rps_info_msg->backup_status = 0;  /* ??? */
            break;

        case RPS_LED:
            msg_len = sizeof(rps_show_led_cmd_t);
            rps_led_msg = (rps_show_led_cmd_t *)buff;

            rps_led_msg->rps_fan_status_index = 
                ledStatusIndex(get_fanfail_led()); 
            rps_led_msg->rps_system_status_index = 
                ledStatusIndex(get_rps_led()); 

            for (i = 0; i < CPLD_MAX_PORT; i++) {
                rps_led_msg->port_no[i] = i; 
                rps_led_msg->rps_port_led_index[i] = 
                    ledStatusIndex(get_port_led(i)); 
            }
            break;

        case RPS_PSU:
            msg_len = sizeof(rps_show_psu_cmd_t);
            rps_psu_msg = (rps_show_psu_cmd_t *)buff;

            psState = getPSStatus();

            for (i = 0; i < CPLD_MAX_PS; i++) {
                rps_psu_msg->psu_slot_id[i] = i; 
                psu_present = (psState & (1<<i*3)) ? 1 : 0; 
                psu_pwr_good = (psState & (1<<(i*3) + 1)) ? 1 : 0; 
                if (!psu_present) {
			rps_psu_msg->status[i] = PSU_EMPTY; 
		} else {
			if (!psu_pwr_good) {
				rps_psu_msg->status[i] = PSU_OFFLINE; 
			} else {

				rps_psu_msg->status[i] = PSU_ONLINE; 
			}
		}
                rps_psu_msg->power_rating[i] = psIndex(getPSID(i)); 
            }
            break;

        case RPS_STATUS:
            msg_len = sizeof(rps_show_status_cmd_t);
            rps_status_msg = (rps_show_status_cmd_t *)buff;

            for (i = 0; i < CPLD_MAX_PORT; i++) {
                rps_status_msg->port_no[i] = i; 
                rps_status_msg->backup_status_index[i] = getPortState(i); 
                rps_status_msg->priority[i] = getPriorityRequested(i); 
                rps_status_msg->power_requested[i] = 
                    powerIndex(getPowerRequested(i)); 
                strncpy(rps_status_msg->sn[i],
                   (char *) switchSN[i], MAX_SN_SZ);
            }
            break;

        case RPS_UPG_STAT:
            msg_len = sizeof(rps_show_upg_cmd_t);
            rps_upg_msg = (rps_show_upg_cmd_t *)buff;

            if (!(strncmp(getenv("upg_stat"), "pass", strlen("pass"))))
                rps_upg_msg->upg_stat = 0;
            else if (!(strncmp(getenv("upg_stat"), "fail-tftp", 
                        strlen("fail-tftp"))))
                rps_upg_msg->upg_stat = 1;
            else if (!(strncmp(getenv("upg_stat"), "fail-flash", 
                        strlen("fail-flash"))))
                rps_upg_msg->upg_stat = 2;

            strncpy(rps_upg_msg->prev_ver, getenv("prev_version"), 
                MAX_VER_LEN );
            strncpy(rps_upg_msg->cur_ver, getenv("cur_version"), 
                MAX_VER_LEN);
            break;

        case RPS_SHOW_SCALE:
            msg_len = sizeof(rps_show_scale_cmd_t);
            rps_scale_msg = (rps_show_scale_cmd_t *)buff;
            if (!(strncmp(getenv("scale"), "yes", strlen("yes"))))
                rps_scale_msg->scale = SCALE_2;
            else
                rps_scale_msg->scale = SCALE_1;
            break;

        case RPS_SHOW_NW:
            msg_len = sizeof(rps_show_nw_cmd_t);
            rps_nw_msg = (rps_show_nw_cmd_t *)buff;

            strncpy(rps_nw_msg->ipaddr, getenv("ipaddr"), 
                strlen(getenv("ipaddr")));
            rps_nw_msg->ipaddr[strlen(getenv("ipaddr"))] = '\0';
            strncpy(rps_nw_msg->netmask, getenv("netmask"), 
                strlen(getenv("netmask")));
            rps_nw_msg->netmask[strlen(getenv("netmask"))] = '\0';
            strncpy(rps_nw_msg->gateway, getenv("gatewayip"), 
                strlen(getenv("gatewayip")));
            rps_nw_msg->gateway[strlen(getenv("gatewayip"))] = '\0';
            break;
    }
    
    data[3] = msg_len + 1; 
    *lenReply = msg_len + 5;
    memmove (&data[MSG_BYTE + 1], buff, msg_len);
    return (result);
}

int32_t 
cmdReceive (int port, uint8_t *data, uint32_t size, uint32_t *len)
{
    uint8_t ch = 1 << (CPLD_MAX_PORT - port - 1);
    int result = 0;

    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    result = i2c_slave_write_wait(CFG_I2C_CPU_SLAVE_ADDRESS,
                                  data, size, len, gwaitms);
    ch = 0;
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    if (gDebugMsg) {
        printf("cmdReceive[%d] = 0x%x, size = %d, %02x %02x %02x %02x %02x"
               " %02x %02x %02x\n",
               port, result, *len, data[0], data[1], data[2], data[3], data[4],
               data[5], data[6], data[7]);
        printf("Rx tick = %d\n", get_timer(0) / gTicksPerMS);
        printf("\n\n");
    }
    return (result);
}

int32_t 
replyTransmit (int port, uint8_t *data, uint32_t len)
{
    uint8_t ch = 1 << (CPLD_MAX_PORT - port - 1);
    int result = 0;

    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    result = i2c_slave_read_wait(CFG_I2C_CPU_SLAVE_ADDRESS, data, len, gwaitms);
    ch = 0;
    i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
    if (gDebugMsg) {
        printf("replyTransmit[%d] = 0x%x, size = %d, %02x %02x %02x %02x"
               " %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
               port, result, len, data[0], data[1], data[2], data[3], data[4],
               data[5], data[6], data[7], 
               data[8], data[9], data[10],
               data[11], data[12], data[13],
               data[14], data[15]);
        printf("Tx tick = %d\n", get_timer(0) / gTicksPerMS);
        printf("\n\n");
    }
    return (result);
}

void 
msgSM (int port, int state)
{
    uint8_t data[256];
    uint32_t len = 0, lenReply = 0;
    int32_t result;
    int32_t tid;
    tData tdata;
    
    switch (state) {
        case POLLING:
            loopCount[port] = 0;
            break;
            
        case I2C_CMD:
            if (result = cmdReceive(port, data, 256, &len)) {
                sendMboxMessage(0, 0, port);
                /* failure, timeout */
                if (result == -1)  /* failure -- reset port i2c */
                    resetPortI2C(port);
                msg_state[port] = POLLING;
            }
            else {
                sendMboxMessage(0, 0, port);
                result = msgProcessor
                    (port, data, len, &cmd[port], &echo[port]);
                if (result != REQ_MSG) {
                    /* bad message or cmd*/
                    msg_state[port] = POLLING;
                }
                else {
                    msg_state[port] = I2C_REQ;
                    tdata.param[0] = port;
                    tdata.param[1] = tdata.param[2] = 0;
                    addTimer(tdata, &tid, ONE_SHOT, 1, 0, msgHandler);
                }
            }
            loopCount[port] = 0;
            break;
            
        case I2C_REQ:
            if (loopCount[port] ++ >= 30) {
                loopCount[port] = 0;
                sendMboxMessage(0, 0, port);
                msg_state[port] = POLLING;
            }
            else {
                tdata.param[0] = port;
                tdata.param[1] = tdata.param[2] = 0;
                addTimer(tdata, &tid, ONE_SHOT, 1, 0, msgHandler);
            }
            break;
            
        case I2C_REPLY:
            memset(data, 0x0, sizeof(data));
            buildReply(port, data, &lenReply, cmd[port], echo[port]);
            replyTransmit(port, data, lenReply);
            sendMboxMessage(0, 0, port);
            msg_state[port] = POLLING;
            cmd[port] = echo[port] = 0;
            loopCount[port] = 0;
            break;
            
        default:
            break;
    }
    if (gDebugMsg) {
        if (state != msg_state[port])
            printf("msgSM[%d] state %d -> %d\n",port, state, msg_state[port]);
    }
}

void 
display_sm (void)
{
    int i;
    uint8_t temp[2];
    int32_t res = 0;
    
    printf("\nPort    state    inbox    outbox\n");

    for (i = 0; i < CPLD_MAX_PORT; i++) {
        res = get_mbox(i, temp);
        printf("%4d    %4d      %02x        %02x\n",
            i, msg_state[i], 
            (res == 0) ? temp[0] : 0xff,
            (res == 0) ? temp[1] : 0xff);
    }
    
}

/* 
 *
 * sm commands
 * 
 */
int 
do_sm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char *cmd = "dump";  /* default command */
    int temp = 0;
    
    if (argc <= 1)
        goto usage;
    
    if (argc >= 2) cmd   = argv[1];

    if (!strncmp(cmd,"dump", 2)) { display_sm(); }
    else if (!strncmp(cmd,"wait", 2)) {
        if (argc >= 3) {
            temp = simple_strtoul(argv[2], NULL, 10); 
            if (temp > 300) {
                gwaitms = temp;
            }
        }
        printf("Rx/Tx wait %d ms\n", gwaitms);
    }
    else if (!strncmp(cmd,"debug", 2)) {
        if (argc > 2) {
            if (!strncmp(argv[2],"on", 2))
                gDebugMsg = 1;
            else
                gDebugMsg = 0;
        }
        printf("SM debug %s.\n", gDebugMsg ? "on" : "off");
    }
    else goto usage;
    return (1);

 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

U_BOOT_CMD(
    sm,    3,  1,  do_sm,
    "sm      - state machine dump command\n",
    "\n"
    "sm dump\n"
    "    - dump state machine\n"
    "sm wait [<ms>] \n"
    "    - slave wait number ms\n"
    "sm debug [on | off] \n"
    "    - SM debug\n"
);

