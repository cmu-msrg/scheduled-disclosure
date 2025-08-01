#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <windows.h>
#include <processthreadsapi.h>
#include "../utils/util.h"

// FIXME based on P-core and E-core
// In the order of P-cores, E-cores, and LPE-cores
// Configured for SMT off, 16 cores
#define CORE_LIST (int[]){9,10,0,11,12,13,1,2,3,4,5,6,7,8,14,15}

DWORD WINAPI __attribute__((noinline)) stress(LPVOID lpParam) {
    uint64_t dummy = 0xf0f0f0f0;
    asm volatile(
        ".align 64\n\t"
        "loop%=:\n\t"
        "mov %0, %%rax\n\t"
        "mov %0, %%rbx\n\t"
        "mov %0, %%rcx\n\t"
        "mov %0, %%rdx\n\t"
        "mov %0, %%rsi\n\t"
        "mov %0, %%rdi\n\t"
        "mov %0, %%r9\n\t"
        "mov %0, %%r10\n\t"
        "mov %0, %%r11\n\t"
        "mov %0, %%r12\n\t"
        "mov %0, %%r13\n\t"
        "mov %0, %%r14\n\t"
        
        "and $0x7fffffff, %%rax\n\t"
        "and $0x7fffffff, %%rbx\n\t"
        "and $0x7fffffff, %%rcx\n\t"
        "and $0x7fffffff, %%rdx\n\t"
        "and $0x7fffffff, %%rsi\n\t"
        "and $0x7fffffff, %%rdi\n\t"
        "and $0x7fffffff, %%r9\n\t"
        "and $0x7fffffff, %%r10\n\t"
        "and $0x7fffffff, %%r11\n\t"
        "and $0x7fffffff, %%r12\n\t"
        "and $0x7fffffff, %%r13\n\t"
        "and $0x7fffffff, %%r14\n\t"
        "jmp loop%=\n\t"

        :
        : "r"(dummy)
        : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14"
    );
    return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Wrong Input! Enter: %s  <cpu_load> \n", argv[0]);
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

    int cpu_load;

    sscanf(argv[1], "%d", &cpu_load);

    HANDLE workers[cpu_load];

    for (int i = 0; i < cpu_load; i++) {
        if (!(workers[i] = CreateThread(NULL, 0, stress, NULL, 0, NULL))) {
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
