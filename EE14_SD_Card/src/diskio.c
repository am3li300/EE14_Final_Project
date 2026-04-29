#include "diskio.h"
#include "ff.h"
#include "sd_spi.h"

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv)
{
        if (pdrv != 0)
                return STA_NOINIT;
        return 0;
}

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE pdrv)
{
        if (pdrv != 0)
                return STA_NOINIT;

        sd_spi_init();

        if (sd_init() != 0) {
                return STA_NOINIT;
        }

        return 0;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

// BYTE pdrv,		Physical drive number to identify the drive
// BYTE *buff,		Data buffer to store read data
// LBA_t sector,	Start sector in LBA
// UINT count		Number of sectors to read

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
        if (pdrv != 0 || count == 0) {
                return RES_PARERR;
        }

        for (UINT i = 0; i < count; i++) {
                if (sd_read_block(sector + i, buff + (512 * i)) != 0) {
                        return RES_ERROR;
                }
        }

        return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
        if (pdrv != 0 || count == 0) {
                return RES_PARERR;
        }

        for (UINT i = 0; i < count; i++) {
                if (sd_write_block(sector + i, buff + (512 * i)) != 0) {
                        return RES_ERROR;
                }
        }

        return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
        if (pdrv != 0)
                return RES_PARERR;

        switch (cmd) {
                case CTRL_SYNC:
                        return RES_OK;

                case GET_SECTOR_SIZE:
                        *(WORD *)buff = 512;
                        return RES_OK;

                case GET_BLOCK_SIZE:
                        *(DWORD *)buff = 1;
                        return RES_OK;

                case GET_SECTOR_COUNT:
                        *(LBA_t *)buff = 0x100000; // temporary fake value
                        return RES_OK;

                default:
                        return RES_PARERR;
        }
}

/*-----------------------------------------------------------------------*/
/* Get FAT timestamp                                                     */
/*-----------------------------------------------------------------------*/

DWORD get_fattime(void)
{
        /*
         * Packed FAT timestamp:
         * bits 31-25: year since 1980
         * bits 24-21: month
         * bits 20-16: day
         * bits 15-11: hour
         * bits 10-5 : minute
         * bits 4-0  : second / 2
         *
         * Fake timestamp: 2026-04-28 12:00:00
         */
        return ((DWORD)(2026 - 1980) << 25) | ((DWORD)4 << 21) |
               ((DWORD)28 << 16) | ((DWORD)12 << 11) | ((DWORD)0 << 5) |
               ((DWORD)0 >> 1);
}