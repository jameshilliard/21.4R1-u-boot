/***********************************************************************
Copyright 2003-2006 Raza Microelectronics, Inc.(RMI). All rights
reserved.
Use of this software shall be governed in all respects by the terms and
conditions of the RMI software license agreement ("SLA") that was
accepted by the user as a condition to opening the attached files.
Without limiting the foregoing, use of this software in source and
binary code forms, with or without modification, and subject to all
other SLA terms and conditions, is permitted.
Any transfer or redistribution of the source code, with or without
modification, IS PROHIBITED,unless specifically allowed by the SLA.
Any transfer or redistribution of the binary code, with or without
modification, is permitted, provided that the following condition is
met:
Redistributions in binary form must reproduce the above copyright
notice, the SLA, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution:
THIS SOFTWARE IS PROVIDED BY Raza Microelectronics, Inc. `AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RMI OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*****************************#RMI_3#***********************************/
#ifndef __DRAM_EYE_FINDER_H__
#define __DRAM_EYE_FINDER_H__
#define RX_EN_SHIFT_AMOUNT                       1    
#define MIN_PASS_THRESHOLD                      60    
#define CALIBRATE_DISABLE_AND_MASK           0xff7
#define CALIBRATE_ENABLE_OR_MASK             0x008
#define CALIBRATE_DISABLE_REG_ADDR               7 /* DDR2_CONFIG_REG_GLB_PARAMS */
#define EF_CMD_STRB_DEFAULT_UNBUF_HV             6
#define EF_CMD_STRB_DEFAULT_UNBUF_LV             9
#define EF_CMD_STRB_DEFAULT_REG                 12
#define EF_CMD_STRB_SWEEP_MAX                   32
#define EF_RXTX_STRB_SWEEP_MAX                  16

#define C0_RX_EN_SHIFT_AMOUNT                    3    
#define C0_MIN_PASS_THRESHOLD                 1000    
#define C0_CALIBRATE_DISABLE_AND_MASK        0xfff
#define C0_CALIBRATE_ENABLE_OR_MASK         0x7000
#define C0_CALIBRATE_DISABLE_REG_ADDR            8 /* DDR2_CLK_CAL_PARAMS */
#define C0_EF_CMD_STRB_DEFAULT_UNBUF_HV       0x18
#define C0_EF_CMD_STRB_DEFAULT_UNBUF_LV       0x24
#define C0_EF_CMD_STRB_DEFAULT_REG            0x30
#define C0_EF_CMD_STRB_SWEEP_MAX               128
#define C0_EF_RXTX_STRB_SWEEP_MAX               64

#define XLS_A0_RX_EN_SHIFT_AMOUNT                3
#define XLS_A0_MIN_PASS_THRESHOLD             1000
#define XLS_A0_CALIBRATE_DISABLE_AND_MASK    0xfff
#define XLS_A0_CALIBRATE_ENABLE_OR_MASK     0x7000
#define XLS_A0_CALIBRATE_DISABLE_REG_ADDR        8 /* DDR2_CLK_CAL_PARAMS */
#define XLS_A0_EF_CMD_STRB_DEFAULT_UNBUF_LV   0x24
#define XLS_A0_EF_CMD_STRB_DEFAULT_REG        0x30
#define XLS_A0_EF_CMD_STRB_SWEEP_MAX           128
#define XLS_A0_EF_RXTX_STRB_SWEEP_MAX           64


#endif
