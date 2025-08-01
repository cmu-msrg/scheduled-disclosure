#include "../utils/util.h"
#include <sys/stat.h>
#include <string.h>
#include <windows.h>
#include <processthreadsapi.h>

#define TSC_FREQ 2995200000 // FIXME

uint64_t static volatile *sending;

struct PARAMS {
	uint64_t *start_time;
	uint64_t interval;
    uint64_t num_samples;
};

DWORD WINAPI sender_thread(LPVOID lpParam)
{
	uint64_t i;
    struct PARAMS *params = (struct PARAMS *)lpParam;
	uint64_t *start_time = params->start_time;
	uint64_t interval = params->interval;
    uint64_t repetitions = params->num_samples;
    HANDLE handle = GetCurrentThread();

    DWORD status = SetThreadIdealProcessor(handle, 15);

    if (status == (DWORD)-1) {
        printf("Failed to set the ideal processor.\n");
    }

    uint8_t avx = 0;

    // Synchronize receiver threads
	while(*sending != 1);
    
    for(i = 0; i < repetitions; i++) {

        uint64_t prev_tsc = get_time();
        uint64_t curr_tsc = prev_tsc;
        // Receive and collect sample every 10ms
        while(curr_tsc - prev_tsc < TSC_FREQ / 100) {
            if (avx == 1) {
                asm volatile(
                // ~ 3000 cycles (P-core) 
                "mov $0x50, %%rdi\n\t"

                "xor %%rbx,%%rbx\n\t"

                "loop:\n\t"
                "add $0x1,%%rbx\n\t"  

                "vfmaddsub132ps %%ymm0, %%ymm1, %%ymm2\n\t"
                "vfmaddsub213ps %%ymm0, %%ymm1, %%ymm3\n\t"
                "vfmaddsub231ps %%ymm0, %%ymm1, %%ymm4\n\t"
                "vfmaddsub132ps %%ymm0, %%ymm1, %%ymm5\n\t"
                "vfmaddsub213ps %%ymm0, %%ymm1, %%ymm6\n\t"
                "vfmaddsub231ps %%ymm0, %%ymm1, %%ymm7\n\t"
                "vfmaddsub132ps %%ymm0, %%ymm1, %%ymm8\n\t"
                "vfmaddsub213ps %%ymm0, %%ymm1, %%ymm9\n\t"
                "vfmaddsub231ps %%ymm0, %%ymm1, %%ymm10\n\t"

                "vfmaddsub132ps %%ymm0, %%ymm1, %%ymm2\n\t"
                "vfmaddsub213ps %%ymm0, %%ymm1, %%ymm3\n\t"
                "vfmaddsub231ps %%ymm0, %%ymm1, %%ymm4\n\t"
                "vfmaddsub132ps %%ymm0, %%ymm1, %%ymm5\n\t"
                "vfmaddsub213ps %%ymm0, %%ymm1, %%ymm6\n\t"
                "vfmaddsub231ps %%ymm0, %%ymm1, %%ymm7\n\t"
                "vfmaddsub132ps %%ymm0, %%ymm1, %%ymm8\n\t"
                "vfmaddsub213ps %%ymm0, %%ymm1, %%ymm9\n\t"
                "vfmaddsub231ps %%ymm0, %%ymm1, %%ymm10\n\t"
                
                "vfmaddsub132ps %%ymm0, %%ymm1, %%ymm2\n\t"
                "vfmaddsub213ps %%ymm0, %%ymm1, %%ymm3\n\t"
                "vfmaddsub231ps %%ymm0, %%ymm1, %%ymm4\n\t"
                "vfmaddsub132ps %%ymm0, %%ymm1, %%ymm5\n\t"
                "vfmaddsub213ps %%ymm0, %%ymm1, %%ymm6\n\t"
                "vfmaddsub231ps %%ymm0, %%ymm1, %%ymm7\n\t"
                "vfmaddsub132ps %%ymm0, %%ymm1, %%ymm8\n\t"
                "vfmaddsub213ps %%ymm0, %%ymm1, %%ymm9\n\t"
                "vfmaddsub231ps %%ymm0, %%ymm1, %%ymm10\n\t"

                "cmp %%rbx,%%rdi\n\t"
                "ja loop\n\t"

                "rdtscp\n\t"				
                "shl $32, %%rdx\n\t"
                "or %%rdx, %%rax\n\t"

                "movq %%rax, %0\n\t" 

                : "=m"(curr_tsc)
                : 
                : "rax", "rdx", "rbx", "rdi", "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7", "ymm8", "ymm9", "ymm10", "memory");
            }
            else {
                asm volatile(            
                // ~ 3000 cycles (E-core) 
                "mov $0x2,%%rdi\n\t"
                
                "xor %%rbx,%%rbx\n\t"

                "loop%=:\n\t"
                "add $0x1,%%rbx\n\t"  

                "pause\n\t"
                "pause\n\t"
                "pause\n\t"
                "pause\n\t"
                            
                "cmp %%rbx, %%rdi\n\t"
                "ja loop%=\n\t"

                "rdtscp\n\t"				
                "shl $32, %%rdx\n\t"
                "or %%rdx, %%rax\n\t"

                "movq %%rax, %0\n\t" 

                : "=m"(curr_tsc)
                :
                : "rdx", "rax", "rdi", "rbx", "memory");
            }
        }

        // Alternate Class ID instruction every interval
        avx = (((curr_tsc-*start_time) / interval) % 2);

    }

	return 0;
	
}


int main(int argc, char *argv[])
{
    // Set QoS to HIGH
	PROCESS_POWER_THROTTLING_STATE PowerThrottling;
	RtlZeroMemory(&PowerThrottling, sizeof(PowerThrottling));
	PowerThrottling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;

	PowerThrottling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
	PowerThrottling.StateMask = 0;

	BOOL result = SetProcessInformation(GetCurrentProcess(), 
                                        ProcessPowerThrottling, 
                                        &PowerThrottling,
                                        sizeof(PowerThrottling));
    if (result) {
    } else {
        printf("Failed to set power throttling. Error: %lu\n", GetLastError());
    }

	// Check arguments
	if (argc != 4) {
		fprintf(stderr, "Wrong Input! Enter channel interval, number of senders, and the number of intervals to collect!\n");
		fprintf(stderr, "Enter: %s <interval> <number of senders> <number of intervals>\n", argv[0]);
		exit(1);
	}

	uint64_t i;
	sending = malloc(sizeof(*sending));
	*sending = 0;

	uint64_t *start_t;
	start_t = malloc(sizeof(*start_t));
	*start_t = 0;

	// Parse channel interval
	uint64_t interval = 1; 
	sscanf(argv[1], "%" PRIu64, &interval);
	if (interval <= 0) {
		printf("Wrong interval! interval should be greater than 0!\n");
		exit(1);
	}

    // Calculate the approximate numper of samples to get the desired number of intervals
    uint64_t num_intervals;
    sscanf(argv[3], "%" PRIu64, &num_intervals);
    uint64_t num_samples = ((interval + (TSC_FREQ / 100) - 1) / (TSC_FREQ / 100))*num_intervals;

	// Parse ntasks (number of sender threads)
	int ntasks;
	sscanf(argv[2], "%d", &ntasks);
	if (ntasks < 0) {
		fprintf(stderr, "ntasks cannot be negative!\n");
		exit(1);
	}

	HANDLE hThreads[ntasks];
	DWORD threadIDs[ntasks];
	struct PARAMS params[ntasks];

	// Start sender threads
	for (int tnum = 0; tnum < ntasks; tnum++) {
		params[tnum].start_time = start_t;
		params[tnum].interval = interval;
        params[tnum].num_samples = num_samples;

        hThreads[tnum] = CreateThread(NULL, 0, sender_thread, &params[tnum], 0, &threadIDs[tnum]);
		
		// Check if threads generated correctly
		if(hThreads[tnum] == NULL) {
			printf("Failed to create thread &d\n", i);
			return 1;
		}
	}

	// Synchronize
	do {
		*start_t = get_time();
	} while ((*start_t % interval) > 100);

	*sending = 1;

	// Wait for the threads to finish
    WaitForMultipleObjects(ntasks, hThreads, TRUE, INFINITE);

	// Close handles
	for(int i = 0; i < ntasks; i++) {
		TerminateThread(hThreads[i], 0);
	}

	free((void*)sending);
	free(start_t);

	return 0;
}

