#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <windows.h>

#include "winring0.h"

union msr_val {
        struct {
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
                DWORD eax;
                DWORD edx;
#else
                DWORD edx;
                DWORD eax;
#endif
        } u;
        uint64_t ull;
};

uint64_t msr_read(uint32_t addr) {
        union msr_val v;
        v.ull = 0;

        if (!Rdmsr(addr, (DWORD *)&v.u.eax, (DWORD *)&v.u.edx)) {
                fprintf(stderr, "%d read msr failed\n", addr);
                return -EIO;
        }
        return ((uint64_t)v.u.edx << 32) | v.u.eax;
}

uint64_t msr_read_tx(uint32_t addr, DWORD_PTR affinity) {
        union msr_val v;
        v.ull = 0;

        if (!RdmsrTx(addr, (DWORD *)&v.u.eax, (DWORD *)&v.u.edx, affinity)) {
                fprintf(stderr, "%d read msr failed\n", addr);
                return -EIO;
        }
        return ((uint64_t)v.u.edx << 32) | v.u.eax;
}