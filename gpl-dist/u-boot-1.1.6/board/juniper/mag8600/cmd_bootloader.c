/*
 * Copyright (c) 2011, Juniper Networks, Inc.
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
 * mag8600 bootloader upgrade
 */
#include <common.h>
#include <command.h>

extern char console_buffer[];

int
do_bootloader (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int         rcode = 0;
    static char response[CFG_CBSIZE] = { 0, };
    int         len;

    DECLARE_GLOBAL_DATA_PTR;

    switch (argc) {
    case 0:
    case 1:
    case 2:
        printf ("Usage:\n%s\n", cmdtp->usage);
        rcode = 1;
        break;
    case 3:
    case 4:
        if (strcmp(argv[1], "upgrade") == 0) {
            uint32_t addr  = (uint32_t)simple_strtoul(argv[3], NULL, 16);
            uint32_t loader_start;
            uint32_t loader_end;
            uint32_t loader_size = CFG_LOADER_SIZE;

            if (strcmp(argv[2], "loader") == 0) {
                switch (gd->board_desc.board_type) {
                case MAG8600_MSC:
                    loader_start = CFG_LOADER_START;
                    loader_end = CFG_LOADER_END;
                    break;
                default:
                    printf(" board not supported\n" );
                    break;
                }

                image_header_t *hdr = (image_header_t *)addr;
                if (hdr->ih_magic == IH_MAGIC) {
                    if (hdr->ih_size >= 
                        (loader_size - sizeof(image_header_t))) {
                        printf("WARNING:Loader larger than its partition!\n");
                        rcode = 1;
                    } else {
                        
                        /* !!! WARNING: Make sure that the image header is 
                         * written in the last *sizeof(image_header_t)* bytes
                         * of the loader partition. Because thats where the 
                         * "bootloader check loader" command looks for the 
                         * image header.
                         *
                         * Move the header to the end of the loader space. 
                         * Currently the loader_size is same as the size of 
                         * the partition. So image header automatically goes 
                         * at the end of the partition. 
                         */
                        addr = shuffle_loader_hdr(addr, loader_size);
                        if (is_valid_loader_image(addr, loader_size)) {
                            if (bootloader_upgrade(addr, loader_start, 
                                                   loader_end, loader_size)) {
                                printf("WARNING:loader upgrade failed\n");
                                rcode = 1;
                            } else {
                                printf("Verifying new loader image...");
                                if (is_valid_loader_image(loader_start, 
                                                          loader_size)) {
                                    printf("OK\n");
                                } else {
                                    rcode = 1;
                                }
                            }
                        } else {
                            rcode = 1;
                        }
                    }
                } else {
                    /* no crc? print the warning and update the image */
                    printf("WARNING: Loader image with missing CRC header!\n"
                           "Do you want to continue? [Y/N] (Default: N) ");

                    memset(response, 0, sizeof(response));
                    flush_console_input();
                    len = readline("");
                    if (len > 0) {
                        strcpy(response, console_buffer);
                        if (response[0] == 'y' || response[0] == 'Y') {
                            if (bootloader_upgrade(addr, loader_start,
                                                   loader_end, loader_size)) {
                                printf("WARNING: Loader upgrade failed\n");
                                rcode = 1;
                            }
                        }
                    } else {
                        /* abort */
                        rcode = 1;
                    }
                }
            }
        } else if (strcmp(argv[1], "check") == 0) { /* bootloader check */
            uint32_t uboot_start = 0;
        
            /* the sanity check of the images on bootflash */
            /* bootloader check u-boot command */
            if (strcmp(argv[2], "u-boot") == 0) { 
                uint32_t allocated_len = CFG_UBOOT_SIZE;
                
                /* bootloader check u-boot backup */
                if (strcmp(argv[3], "backup") == 0) { 
                    uint8_t backup_copy_supported = 0;

                    switch (gd->board_desc.board_type) {
                    case MAG8600_MSC:
                        uboot_start = CFG_SECONDARY_UBOOT_START;
                        break;

                    default:
                        printf(" board not supported\n");
                        return 1;
                    }

                    /* On valhalla family, the backup copy is not supported,
                     * so don't check */
                    if (backup_copy_supported) {
                        printf("Checking for a valid secondary u-boot... ");
                        /* Check the presence & sanity of the backup u-boot */
                        /* But don't check for the size */
                        if (is_valid_uboot_image(uboot_start, 
                                                 allocated_len, 0)) {
                            printf("OK\n");
                        } else {
                            rcode = 1;
                        }
                    } else {
                        printf("Backup copy of u-boot is not supported on"
                               " this platform.\n");
                        rcode = 1;
                    }
                } else if (strcmp(argv[3], "active") == 0) { 
                    /* bootloader check u-boot active */
                    switch (gd->board_desc.board_type) {
                    case MAG8600_MSC:
                        uboot_start = CFG_PRIMARY_UBOOT_START;
                        break;
                    default:
                        printf(" board not supported\n" );
                        return 1;
                    }


                    printf("Checking for a valid primary u-boot... ");
                    /* Check the sanity of the active u-boot */
                    /* But don't check for the size */
                    if (is_valid_uboot_image(uboot_start, allocated_len, 0)) {
                        printf("OK\n");
                    } else {
                        rcode = 1;
                    }
                } else {
                    printf("Usage:\n%s\n", cmdtp->usage);
                }
            } else if (strcmp(argv[2], "loader") == 0) {
                switch (gd->board_desc.board_type) {
                case MAG8600_MSC:
                    uboot_start = CFG_LOADER_START;
                    break;
                default:
                    printf(" board not supported\n" );
                    return 1;
                }
    
                printf("Checking for a valid loader... ");
                if (is_valid_loader_image(uboot_start, CFG_LOADER_SIZE)) {
                    printf("OK\n");
                } else {
                    rcode = 1;
                }
            } else {
                printf("Usage:\n%s\n", cmdtp->usage);
            }
        } else {
            printf("Usage:\n%s\n", cmdtp->usage);
            rcode = 1;
        }
        break;
    case 5:
        if (strcmp(argv[1], "upgrade") == 0) {
            uint32_t uboot_start, uboot_sec_start;
            uint32_t uboot_end;
            uint32_t size = CFG_UBOOT_SIZE;
            uint32_t addr  = (uint32_t)simple_strtoul(argv[4], NULL, 16);
            uint8_t  is_active_upgrade = 0, backup_copy_supported = 0;
            uint8_t  image_has_crc;
            
            if (strcmp(argv[2], "u-boot") == 0) {
                if (strcmp(argv[3], "active") == 0) {
                    is_active_upgrade = 1;
                    switch (gd->board_desc.board_type) {
                    case MAG8600_MSC:
                        uboot_start = CFG_PRIMARY_UBOOT_START;
                        uboot_end = CFG_PRIMARY_UBOOT_END;
                        uboot_sec_start = CFG_SECONDARY_UBOOT_START;
                        backup_copy_supported = 1;
                        break;
                    default:
                        printf(" board not supported\n" );
                        break;
                    }
                } else if (strcmp(argv[3], "backup") == 0) {
                    switch (gd->board_desc.board_type) {
                    case MAG8600_MSC:
                        uboot_start = CFG_SECONDARY_UBOOT_START;
                        uboot_end = CFG_SECONDARY_UBOOT_END;
                        backup_copy_supported = 1;
                        break;
                    default:
                        printf(" board not supported\n" );
                        break;
                    }
                } else {
                    printf("Usage:\n%s\n", cmdtp->usage);
                    rcode = 1;
                }
                
                if (rcode == 0) { 
                    /* 
                     * while upgrading an active u-boot(on asgard platforms), 
                     * make sure that the secondary bootloader is 
                     * fine. If image is corrupt or doesn't contain CRC
                     * print a warning to user before writing the
                     * image.
                     */
                    if (is_active_upgrade 
                        && backup_copy_supported) {
                        printf("Checking sanity of backup u-boot...");
                        if (!is_valid_uboot_image(uboot_sec_start, size, 0)) {
                            printf("Do you want to continue? [Y/N] "
                                   "(Default: N)");
                        
                            rcode = 1; 
                            memset(response, 0, sizeof(response));
                            flush_console_input();
                            len = readline("");
                            if (len > 0) {
                                strcpy(response, console_buffer);
                                if (response[0] == 'y' 
                                    || response[0] == 'Y') {
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
                            memset(response, 0, sizeof(response));
                            rcode = 1;
                            if (len > 0) {
                                strcpy(response, console_buffer);
                                if (response[0] == 'y' || response[0] == 'Y') {
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
                                if (image_has_crc
                                    && (is_active_upgrade 
                                        || (!is_active_upgrade 
                                            && backup_copy_supported))) {
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
           bootloader,  5,  1,  do_bootloader,
           "bootloader - upgrade u-boot\n"
           "bootloader - upgrade loader\n"
           "bootloader - upgrade ushell\n"
           "bootloader - check u-boot\n"
           "bootloader - check loader\n",
           "upgrade u-boot <active/backup>  addr\n"
           "    - copy the active or backup copy of u-boot\n"
           "      from 'addr' in memory to the boot flash\n"
           "upgrade loader addr\n"
           "    - copy the loader from 'addr' in memory to\n"
           "      the boot flash\n"
           "upgrade ushell addr\n"
           "    - copy the ushell from 'addr' in memory to\n"
           "      the boot flash\n"
           "check u-boot <active/backup>\n"
           "check u-boot <active/backup>\n"
           "    - Check the presence and the sanity of the\n"
           "      u-boot on active and backup partitions\n"
           "      respectively\n"
           "check loader\n"
           "    - Check the presence and the sanity of the\n"
           "      secondary loader.\n"
);
