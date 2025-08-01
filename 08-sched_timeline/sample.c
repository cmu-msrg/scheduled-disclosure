#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <windows.h>
#include <processthreadsapi.h>
#include "../utils/util.h"

// 5ms
#define INTERVAL 14976000 // 2995200000Hz FIXME based on TSC frequency
 // 60s
#define NUM_SAMPLES 12000
#define SIZE 0x8000
#define STORE_SIZE 0x80000

struct thread_args {
    INT8 *my_address;
    uint64_t start_rdtsc;
    UINT8 is_high;
};


void __attribute__((noinline)) store(struct thread_args *args) {
    INT8 *my_array = args->my_address;
    uint64_t start_rdtsc = args->start_rdtsc;
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
    for(uint64_t i = 0; i < NUM_SAMPLES; i++) {
        uint64_t lower_bound = start_rdtsc + (i+1) * INTERVAL;
        while(get_time() < lower_bound) {
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
        if(get_time() > start_rdtsc + (i+2) * INTERVAL) {
            *(my_array + i) = -1;
        }
        else {
            *(my_array + i) = GetCurrentProcessorNumber();
        }
    }
    _aligned_free(flip_array);
    return;
}

void __attribute__((noinline)) xor(struct thread_args *args) {
    INT8 *my_array = args->my_address;
    UINT8 is_high = args->is_high;
    uint64_t start_rdtsc = args->start_rdtsc;
    int64_t is_high_mask = ~(is_high - 1); //0xf..f if 1, 0 if 0
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
    for(uint64_t i = 0; i < NUM_SAMPLES; i++) {
        uint64_t lower_bound = start_rdtsc + (i+1) * INTERVAL;
        while(get_time() < lower_bound) {
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
        if(get_time() > start_rdtsc + (i+2) * INTERVAL) {
            *(my_array + i) = -1;
        }
        else {
            *(my_array + i) = GetCurrentProcessorNumber();
        }
    }
    _aligned_free(flip_array);
    return;
}

DWORD WINAPI read_core(LPVOID lpParam) {
    struct thread_args *args = (struct thread_args*) lpParam;
    switch (args->is_high) {
        case 0:
        case 1:
            xor(args);
            break;
        case 2:
            store(args);
            break;
        default:
            return 1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    // Check arguments
	if (argc != 3)
	{
		fprintf(stderr, "Wrong Input! Enter: %s <is_high> <monitor_load> \n", argv[0]);
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

    int is_high;
    int monitor_load;
    sscanf(argv[1], "%d", &is_high);
    sscanf(argv[2], "%d", &monitor_load);

    INT8 thread_cores[monitor_load][NUM_SAMPLES];
    HANDLE monitors[monitor_load];
    struct thread_args args[monitor_load];
    uint64_t start_rdtsc = get_time();

    for (int i = 0; i < monitor_load; i++) {
        args[i].my_address = thread_cores[i];
        args[i].start_rdtsc = start_rdtsc;
        args[i].is_high = is_high;
    }

    for (int i = 0; i < monitor_load; i++) {
        if (!(monitors[i] = CreateThread(NULL, 0, read_core, &args[i], 0, NULL))) {
            perror("monitor thread creation failed");
            exit(EXIT_FAILURE);
        }
        SetThreadIdealProcessor(monitors[i], 15);
        SetThreadPriorityBoost(monitors[i], TRUE);
    }

    for (int i = 0; i < monitor_load; i++) {
        if (WaitForSingleObject(monitors[i], INFINITE) != WAIT_OBJECT_0) {
            perror("monitor thread join failed");
            exit(EXIT_FAILURE);
        }
        CloseHandle(monitors[i]);
    }

    // write to file
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    printf("Complete!: %s", asctime (timeinfo));
    char output_filename[100] = {'\0'};
    sprintf(output_filename, "./core_out/%d_%d_%02d%02d.out", is_high, monitor_load, timeinfo->tm_mon + 1, timeinfo->tm_mday);
	FILE *output_file = fopen((char *)output_filename, "w");

    // write test outputs
    for (int i = 0; i < monitor_load; i++) {
        for (int j = 0; j < NUM_SAMPLES; j++) {
            fprintf(output_file, "%d ", thread_cores[i][j]);
        }
        fprintf(output_file, "\n");
    }

    printf("Data stored in %s!\n", output_filename);

    return 0;
}
