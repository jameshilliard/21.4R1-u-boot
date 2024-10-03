#ifndef __FX_GPIO_H__
#define __FX_GPIO_H__

#define GPIO_CFG_IO_VAL       *((u_int32_t *) 0xbef1800C)
#define GPIO_WRITE_DATA_VAL   *((u_int32_t *) 0xbef18008)

extern void gpio_cfg_io_set(u_int32_t io_mask);
extern void gpio_write_data_set(u_int32_t data_val);

#endif

