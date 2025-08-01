#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <windows.h>
#include <processthreadsapi.h>

// FIXME based on P-core and E-core
#define CORE_LIST (int[]){9,10,0,11,12,13,1,5} // Pcores and 1 Ecore for each cluster

DWORD WINAPI __attribute__((noinline)) stress(LPVOID lpParam) {
    uint64_t flip = 0xffffffffffffffff;
    asm volatile(
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
		"xor %0,%%r15\n\t"

        "jmp loop%=\n\t"

		:
		: "r"(flip)
        : "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15");
    return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Wrong Input! Enter: %s <cpu_load> \n", argv[0]);
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
