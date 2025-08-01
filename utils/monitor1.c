#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <windows.h>
#include <processthreadsapi.h>
#include "./rdmsr/winring0.h"
#include "./dump/inpoutx64.h"
#include "./util.h"


// FIXME
#define PSTATE_SAMPLE_CORE_NUM 8
#define CPU_LIST (int[]){9, 10, 0, 11, 12, 13, 4, 8} // CPU idx of P-cores and 1 E-core for each cluster 
#define TABLE_CPU_LIST (int[]){0, 1, 2, 3, 4, 5, 9, 13} // TD table idx of P-cores and 1 E-core for each cluster 
#define TABLE_SAMPLE_CORE_NUM 14 

#define MEASURE_TIME 60000 
#define MS_INTERVAL 2995200
#define BITMASK_UPDATE_MS 5
#define TABLE_MS 50
#define PSTATE_MS 10
#define NUM_PSTATE_SAMPLES (MEASURE_TIME / PSTATE_MS)
#define NUM_TABLE_SAMPLES (MEASURE_TIME / TABLE_MS)
#define PSTATE_INTERVAL (MS_INTERVAL * PSTATE_MS)
#define TABLE_INTERVAL (MS_INTERVAL * TABLE_MS) 
#define MSR_HF_PTR 0x17d0
#define MSR_PERF_STATUS 0x198

struct thread_arg {
    uint64_t start_rdtsc;
    char *my_pstate;
};

// not to sample P-state of idle cores
// uint16_t zero_bitmask = 0;

DWORD WINAPI __attribute__((noinline)) sample_pstate(LPVOID lpParam) {
    struct thread_arg my_arg = *(struct thread_arg *)lpParam;
    uint64_t start_rdtsc = my_arg.start_rdtsc;
    char *my_pstate = my_arg.my_pstate;
    for(uint64_t i = 0; i < NUM_PSTATE_SAMPLES; i++) {
        my_pstate[i] = (msr_read(MSR_PERF_STATUS) & 0xff00) >> 8;
        while(get_time() < start_rdtsc + (i+1) * PSTATE_INTERVAL) {
            nanosleep((const struct timespec[]){{0, BITMASK_UPDATE_MS * 1000000L}}, NULL);
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 1)
	{
		fprintf(stderr, "Wrong Input! Enter: %s \n", argv[0]);
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

    struct ehfi_table samples[NUM_TABLE_SAMPLES];
    char pstates[PSTATE_SAMPLE_CORE_NUM][NUM_PSTATE_SAMPLES] = {0};
    winring0_init();
    inpoutx64_init();
    HANDLE pstate_sampler[PSTATE_SAMPLE_CORE_NUM];
    struct thread_arg arg[PSTATE_SAMPLE_CORE_NUM];
    uint64_t start_rdtsc = get_time();
    
    uint64_t physmem = msr_read(MSR_HF_PTR)&(~0xfff);
    struct ehfi_table prev_sample, curr_sample;

    for (int i = 0; i < PSTATE_SAMPLE_CORE_NUM; i++) {
        arg[i].start_rdtsc = start_rdtsc;
        arg[i].my_pstate = pstates[i];
        if (!(pstate_sampler[i] = CreateThread(NULL, 0, sample_pstate, &arg[i], 0, NULL))) {
            perror("worker thread creation failed");
            exit(EXIT_FAILURE);
        }
        SetThreadAffinityMask(pstate_sampler[i], 1 << CPU_LIST[i]);
    }

    for(uint64_t i = 0; i < NUM_TABLE_SAMPLES; i++) {
        samples[i] = curr_sample;
        while(get_time() < start_rdtsc + (i+1) * TABLE_INTERVAL) {
            nanosleep((const struct timespec[]){{0, BITMASK_UPDATE_MS * 1000000L}}, NULL);
            read_table(&curr_sample, physmem);
            if(curr_sample.timestamp != prev_sample.timestamp) {
                prev_sample = curr_sample;
                // zero_bitmask = 0;
                // for (int j = 0; j < PSTATE_SAMPLE_CORE_NUM; j++) {
                //     if (curr_sample.cpu[TABLE_CPU_LIST[j]].td_class[0].perf == 0) {
                //         zero_bitmask += (1 << CPU_LIST[j]);
                //     }
                // }
            }
        }
    }
    for (int i = 0; i < PSTATE_SAMPLE_CORE_NUM; i++) {
        WaitForSingleObject(pstate_sampler[i], INFINITE);
        CloseHandle(pstate_sampler[i]);
    }
    winring0_deinit();
    inpoutx64_deinit();

    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    printf("Complete!: %s", asctime (timeinfo));
    char output_filename[100] = {'\0'};
    sprintf(output_filename, "./out/%02d%02d.out", timeinfo->tm_mon + 1, timeinfo->tm_mday);
    FILE *output_file = fopen((char *)output_filename, "w");
    // write ehfi
    for (int iter = 0; iter < NUM_TABLE_SAMPLES; iter++) {
        fprintf(output_file, "%lld:", samples[iter].timestamp);
        for (int i = 0; i < TABLE_SAMPLE_CORE_NUM; i++) {
            for (int j = 0; j < 4; j++) {
                fprintf(output_file, "%d,", samples[iter].cpu[i].td_class[j].perf);
            }
            for (int j = 0; j < 4; j++) {
                fprintf(output_file, "%d,", samples[iter].cpu[i].td_class[j].ee);
            }
        }
        fprintf(output_file, "\n");
    }
    // write pstate
    for (int iter = 0; iter < NUM_PSTATE_SAMPLES; iter++) {
        for (int i = 0; i < PSTATE_SAMPLE_CORE_NUM; i++) {
            fprintf(output_file, "%d,", pstates[i][iter]);
        }
        fprintf(output_file, "\n");
    }
    fclose(output_file);
    printf("Data stored in %s!\n", output_filename);
    return 0;
}