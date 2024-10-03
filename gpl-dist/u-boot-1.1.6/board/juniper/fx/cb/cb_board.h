
#ifndef __CB_BOARD_H__
#define __CB_BOARD_H__

uchar cb_num_i2c_devices[ ];

uchar cb_i2c_bus1_list[ ];

uchar cb_i2c_bus2_list[ ];  

/* Board list of i2c devices on each i2c bus */
uchar *cb_board_i2c_devs[MAX_I2C_BUSES];

#endif
