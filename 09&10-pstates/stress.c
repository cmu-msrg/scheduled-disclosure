#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
// #include <pthread.h>
#include <time.h>
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <signal.h>
#include <windows.h>
#include <processthreadsapi.h>
#include "../utils/util.h"

// 5ms
#define INTERVAL 14976000 // 2995200000Hz FIXME based on TSC frequency
 // 60s
#define NUM_SAMPLES 12000
#define SIZE 0x8000
#define STORE_SIZE 0x80000
#define THREAD_NUM 14
#define LPE_LIST (int[]){14, 15} // FIXME

void __attribute__((noinline)) stress_str() {
    uint64_t flip = 0xffffffffffffffffL;
    uint64_t *flip_array = (uint64_t *)_aligned_malloc(STORE_SIZE * sizeof(uint64_t), 64);
    if (!flip_array) {
        printf("Memory allocation failed!\n");
        return;
    }
    for(size_t i = 0; i < STORE_SIZE; i++) {
        flip_array[i] = flip;
    }
    size_t k = 0;
    volatile uint64_t data0, data1, data2, data3, data4, data5, data6, data7;
    while(1) {
        for(size_t j = 0; j < 5000; j++) {
            flip_array[k] ^= flip;
            flip_array[k+1] ^= flip;
            flip_array[k+2] ^= flip;
            flip_array[k+3] ^= flip;
            flip_array[k+4] ^= flip;
            flip_array[k+5] ^= flip;
            flip_array[k+6] ^= flip;
            flip_array[k+7] ^= flip;
            data0 ^= flip;
            data1 ^= flip;
            data2 ^= flip;
            data3 ^= flip;
            data4 ^= flip;
            data5 ^= flip;
            data6 ^= flip;
            data7 ^= flip;
            k = (k + 8) % STORE_SIZE;
        }
    }
    _aligned_free(flip_array);
    return;
}

void __attribute__((noinline)) stress_xor(int is_high) {
    int64_t is_high_mask = ~(is_high - 1); //0xf..f if 6, 0 if 5
    uint64_t flip = (is_high_mask&0xffffffffffffffffL) | ((~is_high_mask)&0);
    uint64_t *flip_array = (uint64_t *)_aligned_malloc(SIZE * sizeof(uint64_t), 64);
    if (!flip_array) {
        printf("Memory allocation failed!\n");
        return;
    }
    for(size_t i = 0; i < SIZE; i++) {
        flip_array[i] = flip;
    }
    size_t k = 0;
    volatile uint64_t data0, data1, data2, data3, data4, data5, data6, data7;
    data0 = data1 = data2 = data3 = data4 = data5 = data6 = data7 = 0;
    while(1) {
        for(size_t j = 0; j < 5000; j++) {
            data0 ^= flip_array[k];
            data1 ^= flip_array[k+1];
            data2 ^= flip_array[k+2];
            data3 ^= flip_array[k+3];
            data4 ^= flip_array[k+4];
            data5 ^= flip_array[k+5];
            data6 ^= flip_array[k+6];
            data7 ^= flip_array[k+7];
            k = (k + 8) % SIZE;
        }
    }
    _aligned_free(flip_array);
    return;
}

DWORD WINAPI stress(LPVOID lpParam) {
    int inst = *(int*)lpParam % 3;
    // make switch with inst
    switch (inst) {
        case 0:
        case 1:
            stress_xor(inst);
            break;
        case 2:
            stress_str();
            break;
        default:
            break;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    // Check arguments
	if (argc != 2)
	{
		fprintf(stderr, "Wrong Input! Enter: %s <inst> \n", argv[0]);
		exit(EXIT_FAILURE);
	}
    int inst;
    inst = atoi(argv[1]);
    if(inst < 0 || inst > 6) {
        fprintf(stderr, "Wrong Input! Enter: %s <inst> \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    PROCESS_POWER_THROTTLING_STATE PowerThrottling;
	RtlZeroMemory(&PowerThrottling, sizeof(PowerThrottling));
	PowerThrottling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;

	PowerThrottling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
	PowerThrottling.StateMask = 0;

	if (!SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &PowerThrottling, sizeof(PowerThrottling))) {
        printf("Process Information setting failed.\n");
        exit(EXIT_FAILURE);
    }
    HANDLE lpe_pin[2];
    HANDLE workers[THREAD_NUM];


    if(inst > 2) {
        for (int i = 0; i < 2; i++) {
            if (!(lpe_pin[i] = CreateThread(NULL, 0, stress, &inst, 0, NULL))) {
                perror("monitor thread creation failed");
                exit(EXIT_FAILURE);
            }
            SetThreadAffinityMask(lpe_pin[i], 1 << (LPE_LIST[i]));
        }
    }

    for (int i = 0; i < THREAD_NUM; i++) {
        if (!(workers[i] = CreateThread(NULL, 0, stress, &inst, 0, NULL))) {
            perror("monitor thread creation failed");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < THREAD_NUM; i++) {
        if (WaitForSingleObject(workers[i], INFINITE) != WAIT_OBJECT_0) {
            perror("monitor thread join failed");
            exit(EXIT_FAILURE);
        }
        CloseHandle(workers[i]);
    }

    for (int i = 0; i < 2; i++) {
        if (WaitForSingleObject(lpe_pin[i], INFINITE) != WAIT_OBJECT_0) {
            perror("monitor thread join failed");
            exit(EXIT_FAILURE);
        }
        CloseHandle(lpe_pin[i]);
    }
    return 0;
}
