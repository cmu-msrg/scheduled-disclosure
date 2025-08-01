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

struct thread_args {
    INT8 *my_address;
    uint64_t start_rdtsc;
};

DWORD WINAPI read_core(LPVOID lpParam) {
    struct thread_args *args = (struct thread_args*) lpParam;
    INT8 *my_array = args->my_address;
    uint64_t start_rdtsc = args->start_rdtsc;
    uint64_t dummy = 0xf0f0f0f0;
    size_t k = 0;
    volatile uint64_t data0, data1, data2, data3, data4, data5, data6, data7;
    uint64_t flip = 0xffffffffffffffff;
    for(uint64_t i = 0; i < NUM_SAMPLES; i++) {
        uint64_t lower_bound = start_rdtsc + (i+1) * INTERVAL;
        while(get_time() < lower_bound) {
            asm volatile(
                "xor %%r15, %%r15\n\t"
                "xor %%rdx,%%rdx\n\t"
                "xor %%rbx,%%rbx\n\t"
                "xor %%rcx,%%rcx\n\t"
                "xor %%rsi,%%rsi\n\t"
                "xor %%rdi,%%rdi\n\t"
                "xor %%r8,%%r8\n\t"	 
                "xor %%r9,%%r9\n\t"	 
                "xor %%r10,%%r10\n\t"
                "xor %%r11,%%r11\n\t"
                "xor %%r12,%%r12\n\t"
                "xor %%r13,%%r13\n\t"
                "xor %%r14,%%r14\n\t"
                "xor %%r15,%%r15\n\t"

                ".align 64\t\n"
                "loop%=:\n\t"

                "xor %0,%%rdx\n\t"
                "xor %0,%%rbx\n\t"	
                "xor %0,%%rcx\n\t"	
                "xor %0,%%rsi\n\t"	
                "xor %0,%%rdi\n\t"	
                "xor %0,%%r8\n\t"	
                "xor %0,%%r9\n\t"	
                "xor %0,%%r10\n\t"	
                "xor %0,%%r11\n\t"	
                "xor %0,%%r12\n\t"	
                "xor %0,%%r13\n\t"	
                "xor %0,%%r14\n\t"	
                "inc %%r15\n\t"
                "cmp $5000, %%r15\n\t"
                "jge loop%=\n\t"
                :
                : "r"(flip)
                : "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15");
        }
        if(get_time() > start_rdtsc + (i+2) * INTERVAL) {
            *(my_array + i) = -1;
        }
        else {
            *(my_array + i) = GetCurrentProcessorNumber();
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    // Check arguments
	if (argc != 2)
	{
		fprintf(stderr, "Wrong Input! Enter: %s <monitor_load> \n", argv[0]);
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
    int monitor_load;
    sscanf(argv[1], "%d", &monitor_load);

    INT8 thread_cores[monitor_load][NUM_SAMPLES];
    HANDLE monitors[monitor_load];
    struct thread_args args[monitor_load];

    uint64_t start_rdtsc = get_time();
    for (int i = 0; i < monitor_load; i++) {
        args[i].my_address = thread_cores[i];
        args[i].start_rdtsc = start_rdtsc;
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
    sprintf(output_filename, "./core_out/%d_%02d%02d.out", monitor_load, timeinfo->tm_mon + 1, timeinfo->tm_mday);
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
