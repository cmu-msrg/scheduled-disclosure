#include "../utils/util.h"
#include <sys/stat.h>
#include <string.h>
#include <windows.h>
#include <processthreadsapi.h>

#define TSC_FREQ 2995200000 // FIXME

uint64_t static volatile *receiving;

struct PARAMS {
    int num_thread;
    char directory[100];
    uint64_t num_samples;
};

DWORD WINAPI receiver_thread(void* lpParam)
{
    uint64_t i;
    struct PARAMS *params = (struct PARAMS *)lpParam;
    int num_thread = params->num_thread;
    uint64_t repetitions = params->num_samples;
    HANDLE handle = GetCurrentThread();

    DWORD status = SetThreadIdealProcessor(handle, 15);

    if (status == (DWORD)-1) {
        printf("Failed to set the ideal processor.\n");
    }
    
    char filename[100];
    sprintf(filename, strcat(params->directory, "/receiver%d.out"), num_thread);

    FILE *output_file = fopen(filename, "w");

    DWORD *result_cpu = (DWORD *)malloc(sizeof(*result_cpu) * repetitions);
    uint64_t *timestamps = (uint64_t *)malloc(sizeof(*timestamps) * repetitions);

    // Synchronize receiver threads
    while(*receiving != 1);

    for(i = 0; i < repetitions; i++) {

        uint64_t prev_tsc = get_time();
        uint64_t curr_tsc = prev_tsc;
        // Receive and collect sample every 10ms
        while(curr_tsc - prev_tsc < (TSC_FREQ / 100)) {
            asm volatile(            
            // ~ 3000 cycles (P-core)
            "mov $0x1E0,%%rdi\n\t" 

            "xor %%rbx,%%rbx\n\t"

            "loop:\n\t"
            "add $0x1,%%rbx\n\t"

            // Clear add destination registers
            "xor %%r8, %%r8\n\t"
            "add $5, %%r8\n\t"
            "inc %%r8\n\t"
            "xor %%r9, %%r9\n\t"
            "add $5, %%r9\n\t"
            "inc %%r9\n\t"
            "xor %%r10, %%r10\n\t"
            "add $5, %%r10\n\t"
            "inc %%r10\n\t"
            "xor %%r11, %%r11\n\t"
            "add $5, %%r11\n\t"
            "inc %%r11\n\t"

            "cmp %%rbx, %%rdi\n\t"
            "ja loop\n\t"

            "rdtscp\n\t"				
            "shl $32, %%rdx\n\t"
            "or %%rdx, %%rax\n\t"

            "movq %%rax, %0\n\t"

            : "=m"(curr_tsc)
            : 
            : "rax", "rdx", "rdi", "rbx", "r8", "r9", "r10", "r11", "memory");
        }
        result_cpu[i] = GetCurrentProcessorNumber();
        timestamps[i] = curr_tsc;
    }


	// Store the samples to disk
	for (i = 0; i < repetitions; i++) {
        fprintf(output_file, "%" PRIu64 " %lu\n", timestamps[i], result_cpu[i]);
    }

	// Free the buffers and file
	fclose(output_file);
    free(result_cpu);
    free(timestamps);

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
		fprintf(stderr, "Wrong Input! Enter channel interval, number of receivers, and the number of intervals to collect!\n");
		fprintf(stderr, "Enter: %s <interval> <number of receivers> <number of intervals>\n", argv[0]);
		exit(1);
	}

	uint64_t i;
    receiving = malloc(sizeof(*receiving));
	*receiving = 0;

    // Create directory
	char dir_path[100];
	sprintf(dir_path, "./out");
    struct stat st;
    if (stat(dir_path, &st) == 0 && S_ISDIR(st.st_mode)) {
        ;
    } else {
        if (mkdir(dir_path) != 0) {
            fprintf(stderr, "Failed to create directory!\n");
            exit(1);
		}
	}

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

    // Parse ntasks (number of receiver threads)
	int ntasks;
	sscanf(argv[2], "%d", &ntasks);
	if (ntasks < 0) {
		fprintf(stderr, "ntasks cannot be negative!\n");
		exit(1);
	}

    HANDLE hThreads[ntasks];
	DWORD threadIDs[ntasks];
    struct PARAMS params[ntasks];

	// Start receiver threads
	for (int tnum = 0; tnum < ntasks; tnum++) {
        params[tnum].num_thread = tnum;
        strcpy(params[tnum].directory, dir_path);
        params[tnum].num_samples = num_samples;
    
        hThreads[tnum] = CreateThread(NULL, 0, receiver_thread, &params[tnum], 0, &threadIDs[tnum]);

		// Check if threads generated correctly
		if(hThreads[tnum] == NULL) {
			printf("Failed to create thread &d\n", i);
			return 1;
		}
	}

     // Synchronize
	uint64_t start_t;

	do {
		start_t = get_time();
	} while ((start_t % interval) > 100);
 
	*receiving = 1;

    // Wait for the threads to finish
    WaitForMultipleObjects(ntasks, hThreads, TRUE, INFINITE);

    // Close handles
	for(int i = 0; i < ntasks; i++) {
		TerminateThread(hThreads[i], 0);
	}

    free((void*)receiving);

    return 0;
}
