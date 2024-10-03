#ifndef __ASM_MACROS_H__
#define __ASM_MACROS_H__

#define SAVE32_COP0_REG(reg, id, sel) mtc0 reg, _(id),sel
#define RESTORE32_COP0_REG(reg, id, sel) mfc0 reg, _(id),sel

#endif

