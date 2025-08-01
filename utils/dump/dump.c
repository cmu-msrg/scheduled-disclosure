#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

#include <windows.h>
#include <winternl.h>
#include <winerror.h>
#include <psapi.h>

#include "inpoutx64.h"

#define DRV_FILE        L"inpoutx64.sys"
#define TD_SIZE         144

int read_table(struct ehfi_table *table, uint64_t physmem)
{
        static HANDLE map_hdl;
        static uint8_t *virt_addr = NULL;
        if (!is_inpoutx64_driver_open()) {
                printf("Failed to open Inpoutx64 driver\n");
                inpoutx64_deinit();

                return -EIO;
        }

        if(virt_addr == NULL) {
                virt_addr = MapPhysToLin((PBYTE)physmem, 8, &map_hdl);
                if (virt_addr == NULL) {
                        printf("failed to mmap phys addr 0x%016jx\n", physmem);
                        return -EIO;
                }
        }

        // read from virt_addr contents to ehfi_table
        memcpy(table, virt_addr, sizeof(struct ehfi_table));

        // UnmapPhysicalMemory(map_hdl, virt_addr);

        return 0;
}
