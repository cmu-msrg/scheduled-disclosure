#include "../utils/util.h"
#include <sys/stat.h>
#include <string.h>
#include <windows.h>
#include <processthreadsapi.h>

#define TSC_FREQ 2995200000
#define INTERVAL (TSC_FREQ / 100) // 10ms

uint64_t static volatile *receiving;

struct PARAMS {
    int num_thread;
    char directory[100];
    int input;
   };

DWORD WINAPI receiver_thread(void* lpParam)
{
    int i;
    struct PARAMS *params = (struct PARAMS *)lpParam;
    int num_thread = params->num_thread;
    int input = params->input;
    HANDLE handle = GetCurrentThread();

    // set to unused core (e.g., LPE core)
    DWORD status = SetThreadIdealProcessor(handle, 16);

    if (status == (DWORD)-1) {
        printf("failed to set the ideal processor.\n");
    }

    char filename[100];
    sprintf(filename, strcat(params->directory, "/class3-%d-%d.out"), num_thread, input);

    FILE *output_file = fopen(filename, "w");

	const int repetitions = 10000;

    DWORD *result_cpu = (DWORD *)malloc(sizeof(*result_cpu) * repetitions);
    uint64_t *timestamps = (uint64_t *)malloc(sizeof(*timestamps) * repetitions);

    while(*receiving != 1);

    for(i = 0; i < repetitions; i++) {

        uint64_t prev_tsc = get_time();
        uint64_t curr_tsc = prev_tsc;
        // Receive and collect sample every 10ms
        while(curr_tsc - prev_tsc < INTERVAL) {
            asm volatile(            
            // ~ 3000 cycles (E-core)
            "mov $0x2,%%rdi\n\t"
            
            "xor %%rbx,%%rbx\n\t"

            "loop:\n\t"
            "add $0x1,%%rbx\n\t"  

            "pause\n\t"
            "pause\n\t"
            "pause\n\t"
            "pause\n\t"
                           
            "cmp %%rbx, %%rdi\n\t"
            "ja loop\n\t"

            "rdtscp\n\t"				
            "shl $32, %%rdx\n\t"
            "or %%rdx, %%rax\n\t"

            "movq %%rax, %0\n\t"

            : "=m"(curr_tsc)
            :
            : "rdx", "rax", "rdi", "rbx");
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
	if (argc != 3) {
		fprintf(stderr, "Wrong Input! Enter number of receivers!\n");
		fprintf(stderr, "Enter: %s <number of receivers>\n", argv[0]);
		exit(1);
	}

    // Parse the third argument
    int input;
    sscanf(argv[2], "%d", &input);

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

    // Parse ntasks
	int ntasks;
	sscanf(argv[1], "%d", &ntasks);
	if (ntasks < 0) {
		fprintf(stderr, "ntasks cannot be negative!\n");
		exit(1);
	}

    HANDLE hThreads[ntasks];
	DWORD threadIDs[ntasks];
    struct PARAMS params[ntasks];

	// Start victim threads
	for (int tnum = 0; tnum < ntasks; tnum++) {
        params[tnum].num_thread = tnum;
        strcpy(params[tnum].directory, dir_path);
        params[tnum].input = input;
    
        hThreads[tnum] = CreateThread(NULL, 0, receiver_thread, &params[tnum], 0, &threadIDs[tnum]);

		// Check if threads generated correctly
		if(hThreads[tnum] == NULL) {
			printf("Failed to create thread %d\n", tnum);
			return 1;
		}
	}

    // Wait 1ms
	uint64_t start_t = get_time();
    while ((get_time() - start_t) < (TSC_FREQ / 1000)){}
 
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
