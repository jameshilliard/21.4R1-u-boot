/*
 * Copyright (c) 2014, Juniper Networks, Inc.
 * All rights reserved.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. if not ,see
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0,html>
 */

/*
 * acx500_svcs bootloader upgrade
 */
#include <common.h>
#include <command.h>

extern char console_buffer[];

int
get_uboot_bank (void)
{
    uint8_t off = ACX500_SVCS_FLASH_SWIZZLE;
    uint8_t group = ACX500_SVCS_I2C_GRP;
    uint8_t val = 0;
    uint8_t dev_addr = ACX500_SVCS_CPLD;

    if (!read_i2c(dev_addr, off, &val, group)) {
        return -1;
    }

    return val & ACX500_SVCS_FLASH_BANK ? 1 : 0;
}

static int
set_uboot_bank (int bank)
{
    uint8_t off = ACX500_SVCS_FLASH_SWIZZLE;
    uint8_t group = ACX500_SVCS_I2C_GRP;
    uint8_t val = 0;
    uint8_t dev_addr = ACX500_SVCS_CPLD;

    if (!read_i2c(dev_addr, off, &val, group)) {
        return -1;
    }

    val &= ~ACX500_SVCS_FLASH_BANK;
    val |= (bank << ACX500_SVCS_FLASH_BANK_SHIFT);

    if (!write_i2c(dev_addr, off, val, group)) {
        return -1;
    }

    return 0;
}

int 
do_activebank (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int bank;

    if (argc == 1) {
        printf("Curently Active Logical Bank = bank%d\n", get_uboot_bank());
        return 0;
    } else if (argc == 2) {
        printf("Setting Currently Active Logical Bank to %s...\n", argv[1]);

        if (strncmp(argv[1], "bank0", 5) == 0) {
            bank = 0;
        } else if (strncmp(argv[1], "bank1", 5) == 0) {
            bank = 1;
        } else {
            goto usage;
        }

        if (set_uboot_bank(bank) == -1) {
            puts("Error trying to set the Boot Bank\n");
            return 1;
        }

        if ((bank = get_uboot_bank()) == -1) {
            puts("Error trying to read the Boot Bank\n");
            return 1;
        } else {
            printf("Curently Active Logical Bank = bank%d\n", bank);
        }

        return 0;
    }

usage: 
    printf("Usage:\nactivebank\n");
    printf("%s\n", cmdtp->help);

    return 1;
}

int
do_bootloader (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int         rcode = 0;
    int         len, current_bank;
    uint8_t     uboot_bank;
    uint32_t    uboot_start, uboot_end;
    uint32_t    size;
    uint32_t    addr;
    uint32_t    allocated_len;
    uint8_t     image_has_crc = 0;

    DECLARE_GLOBAL_DATA_PTR;

    switch (argc) {
    case 0:
    case 1:
    case 2:
    case 3:
        printf ("Usage:\n%s\n", cmdtp->usage);
        rcode = 1;
        break;
    case 4:
        if (strcmp(argv[1], "check") == 0) {

            /* the sanity check of the images on bootflash */
            if (strcmp(argv[2], "u-boot") == 0) {
                uboot_start = CFG_UBOOT_START;
                allocated_len = CFG_UBOOT_SIZE;

                /* bootloader check u-boot backup */
                if (strcmp(argv[3], "backup") == 0) {
                    /* Get the current u-boot bank first */
                    if ((current_bank = get_uboot_bank()) == -1) {
                        puts("Failed to read the current bank\n");
                        puts("Aborting u-boot upgrade...\n");
                        return 1;
                    }

                    if (set_uboot_bank(UBOOT_BANK_1) == -1) {
                        puts("Unable to set u-boot bank to bank1\n");
                        puts("Aborting u-boot check...\n");
                        return 1;
                    }
                    printf("Checking for a valid secondary u-boot... ");

                    /* Check the presence & sanity of the backup u-boot */
                    if (is_valid_uboot_image(uboot_start, allocated_len, 0)) {
                        printf("OK\n");
                    } else {
                        rcode = 1;
                    }
                    /* Revert back to the current bank */
                    set_uboot_bank(current_bank);
                } else if (strcmp(argv[3], "active") == 0) {
                    /* Get the current u-boot bank first */
                    if ((current_bank = get_uboot_bank()) == -1) {
                        puts("Failed to read the current bank\n");
                        puts("Aborting u-boot upgrade...\n");
                        return 1;
                    }

                    if (set_uboot_bank(UBOOT_BANK_0) == -1) {
                        puts("Unable to set u-boot bank to bank0\n");
                        puts("Aborting u-boot check...\n");
                        return 1;
                    }

                    printf("Checking for a valid primary u-boot... ");

                    /* Check the sanity of the active u-boot */
                    if (is_valid_uboot_image(uboot_start, allocated_len, 0)) {
                        printf("OK\n");
                    } else {
                        rcode = 1;
                    }
                    /* Revert back to the current bank */
                    set_uboot_bank(current_bank);
                } else {
                    printf("Usage:\n%s\n", cmdtp->usage);
                    rcode = 1;
                }
            } else {
                printf("Usage:\n%s\n", cmdtp->usage);
                rcode = 1;
            }
        }

        break;

    case 5:
        if (strcmp(argv[1], "upgrade") == 0) {
            if (strcmp(argv[2], "u-boot") == 0) {
                size = CFG_UBOOT_SIZE;
                uboot_start = CFG_UBOOT_START;
                uboot_end = CFG_UBOOT_END;
                addr  = (uint32_t)simple_strtoul(argv[4], NULL, 16);

                if (strcmp(argv[3], "active") == 0) {
                    /* Get the current u-boot bank first */
                    if ((current_bank = get_uboot_bank()) == -1) {
                        puts("Failed to read the current bank\n");
                        puts("Aborting u-boot upgrade...\n");
                        return 1;
                    }

                    uboot_bank = UBOOT_BANK_0;
                    if (set_uboot_bank(UBOOT_BANK_0) == -1) {
                        puts("Unable to set u-boot bank to bank0\n");
                        puts("Aborting upgrade...\n");
                        return 1;
                    }
                } else if (strcmp(argv[3], "backup") == 0) {
                    /* Get the current u-boot bank first */
                    if ((current_bank = get_uboot_bank()) == -1) {
                        puts("Failed to read the current bank\n");
                        puts("Aborting u-boot upgrade...\n");
                        return 1;
                    }

                    uboot_bank = UBOOT_BANK_1;
                    if (set_uboot_bank(UBOOT_BANK_1) == -1) {
                        puts("Unable to set u-boot bank to bank1\n");
                        puts("Aborting upgrade...\n");
                        return 1;
                    }
                } else {
                    printf("Usage:\n%s\n", cmdtp->usage);
                    rcode = 1;
                }

                if (rcode == 0) {
                    /*
                     * while upgrading an active u-boot make sure that
                     * the secondary bootloader is fine. If image is
                     * corrupt or doesn't contain CRC print a warning to
                     * user before writing the image.
                     */
                    if (uboot_bank == UBOOT_BANK_0) {
                        printf("Checking sanity of backup u-boot...");
                        if (!is_valid_uboot_image(uboot_start, size, 0)) {
                            printf("Do you want to continue? [Y/N] "
                                   "(Default: N)");

                            rcode = 1;
                            flush_console_input();
                            len = readline("");
                            if (len > 0) {
                                if (console_buffer[0] == 'y' 
                                    || console_buffer[0] == 'Y') {
                                    rcode = 0;
                                }
                            }
                        } else {
                            printf("OK\n");
                        }
                    }

                    if (!rcode) {
                        image_header_t* hdr = 
                            (image_header_t *)(addr + IMG_HEADER_OFFSET);

                        /*
                         * If image has a CRC header but CRC fails, then we
                         * won't write anything. If image doesn't have a CRC
                         * header, we will complain to user but write the 
                         * image.
                         */
                        if (hdr->ih_magic == IH_MAGIC) {
                            image_has_crc = 1;
                            /* 
                             * Before upgrading check if the source address 
                             * has a valid u-boot image. Then upgrade the 
                             * image on bootflash. While upgrading make sure 
                             * that size of the image can fit in the 
                             * partition.
                             */
                            if (!is_valid_uboot_image(addr, size, 1)) {
                                rcode = 1;
                            }
                        } else {
                            printf("WARNING: U-boot image with missing "
                                   "CRC header!\nDo you want to continue?"
                                   "[Y/N] (Default: N)");

                            flush_console_input();
                            len = readline("");
                            rcode = 1;
                            if (len > 0) {
                                if (console_buffer[0] == 'y' || console_buffer[0] == 'Y') {
                                    rcode = 0;
                                }
                            }
                            image_has_crc = 0;
                        }

                        /* write the image, if no previous errors */
                        if (!rcode) {
                            if (bootloader_upgrade(addr, 
                                                   uboot_start, 
                                                   uboot_end, size)) {
                                printf("WARNING: U-Boot upgrade failed\n");
                                rcode = 1;
                            }

                            if (!rcode) {
                                if (image_has_crc) {
                                    /* 
                                     * check if image was written 
                                     * correctly 
                                     */
                                    printf("Verifying the new u-boot... ");
                                    if (!is_valid_uboot_image(uboot_start, 
                                                              size, 0)) {
                                        rcode = 1;
                                    } else {
                                        printf("OK\n");
                                    }
                                }
                            }
                        }
                    }
                }
                /* Revert back to the current bank */
                set_uboot_bank(current_bank);
            } else {
                printf("Usage:\n%s\n", cmdtp->usage);
                rcode = 1;
            }
        } else {
            printf("Usage:\n%s\n", cmdtp->usage);
            rcode = 1;
        }
        break;
    default: 
        printf("Usage:\n%s\n", cmdtp->usage);
        rcode = 1;
        break;
    }
    return rcode;
}

U_BOOT_CMD(
           bootloader,  5,  0,  do_bootloader,
           "bootloader - upgrade u-boot\n"
           "bootloader - check u-boot\n",
           "upgrade u-boot <active/backup>  addr\n"
           "    - copy the active or backup copy of u-boot\n"
           "      from 'addr' in memory to the boot flash\n"
           "check u-boot <active/backup>\n"
           "    - Check the presence and the sanity of the\n"
           "      u-boot on active and backup partitions\n"
           "      respectively\n"
);

U_BOOT_CMD(
	   activebank,	2,	0,	do_activebank,
	   "activebank    - Report / change the Active Flash Logical Bank\n",
	   "\n         - Report the current Active logical bank\n"
	   "activebank bank0\n"
	   "         - Make the bank0 as the currently active bank\n"
	   "activebank bank1\n"
	   "         - Make the bank1 as the currently active bank\n"
	   );
