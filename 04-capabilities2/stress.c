#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <windows.h>
#include <processthreadsapi.h>
#include "../utils/util.h"

// FIXME based on P-core and E-core
// In the orde of P-cores, E-cores, and LPE-cores
// Configured for SMT off, 16 cores
#define CORE_LIST (int[]){9,10,0,11,12,13,1,2,3,4,5,6,7,8,14,15}
#define SIZE 0x8000
#define STORE_SIZE 0x80000

void __attribute__((noinline)) asm_imul() {
    uint64_t dummy = 0xf0f0f0f0;
    uint64_t *my_array = (uint64_t *)_aligned_malloc(SIZE * sizeof(uint64_t), 64);
    if (!my_array) {
        printf("Memory allocation failed!\n");
        return;
    }
    for(size_t i = 0; i < SIZE; i++) {
        my_array[i] = dummy;
    }
    volatile uint64_t data0, data1, data2, data3, data4, data5, data6, data7; // do not optimize
    while (1) {
        for(size_t i = 0; i < SIZE; i+=8) {
            data0 = dummy;
            data1 = dummy;
            data2 = dummy;
            data3 = dummy;
            data4 = dummy;
            data5 = dummy;
            data6 = dummy;
            data7 = dummy;
            data0 *= my_array[i];
            data1 *= my_array[i+1];
            data2 *= my_array[i+2];
            data3 *= my_array[i+3];
            data4 *= my_array[i+4];
            data5 *= my_array[i+5];
            data6 *= my_array[i+6];
            data7 *= my_array[i+7];
        }
    }
    _aligned_free(my_array);
}

void __attribute__((noinline)) asm_and() {
    uint64_t dummy = 0xf0f0f0f0;
    uint64_t *my_array = (uint64_t *)_aligned_malloc(SIZE * sizeof(uint64_t), 64);
    if (!my_array) {
        printf("Memory allocation failed!\n");
        return;
    }
    for(size_t i = 0; i < SIZE; i++) {
        my_array[i] = dummy;
    }
    volatile uint64_t data0, data1, data2, data3, data4, data5, data6, data7; // do not optimize
    while (1) {
        for(size_t i = 0; i < SIZE; i+=8) {
            data0 = 0x7fffffffL;
            data1 = 0x7fffffffL;
            data2 = 0x7fffffffL;
            data3 = 0x7fffffffL;
            data4 = 0x7fffffffL;
            data5 = 0x7fffffffL;
            data6 = 0x7fffffffL;
            data7 = 0x7fffffffL;
            data0 &= my_array[i];
            data1 &= my_array[i+1];
            data2 &= my_array[i+2];
            data3 &= my_array[i+3];
            data4 &= my_array[i+4];
            data5 &= my_array[i+5];
            data6 &= my_array[i+6];
            data7 &= my_array[i+7];
        }
    }
    // never reached
    _aligned_free(my_array);
}

void __attribute__((noinline)) asm_xor(uint64_t cpu_inst) {
    int64_t is_high_mask = ~(cpu_inst - 3); //0xf..f if 3, 0 if 2
    uint64_t flip = (is_high_mask&0xffffffffffffffffL) | ((~is_high_mask)&0);
    uint64_t *my_array = (uint64_t *)_aligned_malloc(SIZE * sizeof(uint64_t), 64);
    if (!my_array) {
        printf("Memory allocation failed!\n");
        return;
    }
    for(size_t i = 0; i < SIZE; i++) {
        my_array[i] = flip;
    }
    volatile uint64_t data0, data1, data2, data3, data4, data5, data6, data7; // do not optimize
    while (1) {
        for(size_t i = 0; i < SIZE; i+=8) {
            data0 ^= my_array[i];
            data1 ^= my_array[i+1];
            data2 ^= my_array[i+2];
            data3 ^= my_array[i+3];
            data4 ^= my_array[i+4];
            data5 ^= my_array[i+5];
            data6 ^= my_array[i+6];
            data7 ^= my_array[i+7];
        }
    }
    _aligned_free(my_array);
}

void __attribute__((noinline)) asm_store() {
    uint64_t flip = 0xffffffffffffffffL;
    uint64_t *my_array = (uint64_t *)_aligned_malloc(STORE_SIZE * sizeof(uint64_t), 64);
    if (!my_array) {
        printf("Memory allocation failed!\n");
        return;
    }
    for(size_t i = 0; i < STORE_SIZE; i++) {
        my_array[i] = flip;
    }
    volatile uint64_t data0, data1, data2, data3, data4, data5, data6, data7; // do not optimize
    while (1) {
        for(size_t i = 0; i < STORE_SIZE; i+=8) {
            my_array[i] ^= flip;
            my_array[i+1] ^= flip;
            my_array[i+2] ^= flip;
            my_array[i+3] ^= flip;
            my_array[i+4] ^= flip;
            my_array[i+5] ^= flip;
            my_array[i+6] ^= flip;
            my_array[i+7] ^= flip;
            data0 ^= flip;
            data1 ^= flip;
            data2 ^= flip;
            data3 ^= flip;
            data4 ^= flip;
            data5 ^= flip;
            data6 ^= flip;
            data7 ^= flip;
        }
    }
    _aligned_free(my_array);
}

DWORD WINAPI __attribute__((noinline)) stress(LPVOID lpParam) {
    int cpu_inst = *(int*)lpParam;
    switch(cpu_inst) {
        case 0:
        asm_imul();
        break;
        case 1:
        asm_and();
        break;
        case 2:
        case 3:
        asm_xor(cpu_inst);
        break;
        case 4:
        asm_store();
        break;
        default:
        break;
    }
    return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		fprintf(stderr, "Wrong Input! Enter: %s  <cpu_inst> <cpu_load> \n", argv[0]);
		exit(EXIT_FAILURE);
	}
    // QoS high
    PROCESS_POWER_THROTTLING_STATE PowerThrottling;
	RtlZeroMemory(&PowerThrottling, sizeof(PowerThrottling));
	PowerThrottling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;

	PowerThrottling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
	PowerThrottling.StateMask = 0;

	if (!SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &PowerThrottling, sizeof(PowerThrottling))) {
        printf("Process Information setting failed.\n");
        exit(EXIT_FAILURE);
    }

    int cpu_inst;
    int cpu_load;

    sscanf(argv[1], "%d", &cpu_inst);
    sscanf(argv[2], "%d", &cpu_load);

    HANDLE workers[cpu_load];

    for (int i = 0; i < cpu_load; i++) {
        if (!(workers[i] = CreateThread(NULL, 0, stress, &cpu_inst, 0, NULL))) {
            perror("worker thread creation failed");
            exit(EXIT_FAILURE);
        }
        SetThreadAffinityMask(workers[i], 1 << (CORE_LIST[i]));
    }
    for (int i = 0; i < cpu_load; i++) {
        if (WaitForSingleObject(workers[i], INFINITE) != WAIT_OBJECT_0) {
            perror("worker thread join failed");
            exit(EXIT_FAILURE);
        }
        CloseHandle(workers[i]);
    }

    return 0;
}
