#ifndef __DEBUG_H__
#define __DEBUG_H__

#define RB_ERR_INFO 0
#define RB_DEBUG    1

extern int user_verbosity_level;

#define core_printf(level, fmt, args...)  { \
	if(level < user_verbosity_level) \
		printf(fmt, ##args); \
	}
	

#define printf_err(fmt, arg...)    \
		do { core_printf(RB_ERR_INFO, fmt, ##arg); }while(0);
#define printf_info(fmt, arg...)    \
		do { core_printf(RB_ERR_INFO, fmt, ##arg); }while(0);
#define printf_debug(fmt, arg...)   \
		do { core_printf(RB_DEBUG,    fmt, ##arg); }while(0);

extern void printf (const char *fmt, ...);

#ifdef SPD_DO_PRINTF
#define SPD_PRINTF(fmt,args...)     printf(fmt,##args)
#else
#define SPD_PRINTF(fmt,args...)
#endif

#ifdef EE_PRINTF
#define EEPROM_PRINTF(fmt,args...)     printf(fmt,##args)
#else
#define EEPROM_PRINTF(fmt,args...)
#endif

#endif

