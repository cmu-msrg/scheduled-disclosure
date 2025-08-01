#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <time.h>

#include <windows.h>

#define FILEPATH_FMUL "../utils/opencl/stress_fmul.cl"
#define FILEPATH_FMA "../utils/opencl/stress_fma.cl"
#define FUNCNAME "stress"

// https://stackoverflow.com/questions/33010010/how-to-generate-random-64-bit-unsigned-integer-in-c
uint64_t rand_uint64(void) {
    uint64_t r = 0;
    for (int i=0; i<64; i += 15 /*30*/) {
        r = r*((uint64_t)RAND_MAX + 1) + rand();
    }
    return r;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: ./run_opencl <type>\n");
        exit(1);
    }

    int type;
    sscanf(argv[1], "%d", &type);

    cl_device_id device;
    cl_context context;
    cl_program program;
    cl_kernel kernel;
    cl_command_queue queue;
    cl_int err;
    size_t local_size, global_size;
    cl_platform_id platform;

    /* Identify a platform */
    err = clGetPlatformIDs(1, &platform, NULL);
    if(err < 0) {
        perror("Couldn't identify a platform");
        exit(1);
    } 

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if(err < 0) {
        perror("Couldn't access GPU");
        exit(1);   
    }
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if(err < 0) {
        perror("Couldn't create a context");
        exit(1);   
    }

    FILE *fp;
    switch (type) {
        case 0:
        fp = fopen(FILEPATH_FMUL, "rb");
        break;
        case 1:
        case 2:
        fp = fopen(FILEPATH_FMA, "rb");
        break;
        default:
            return 0;
    }

    size_t program_size, log_size;
    fseek(fp, 0, SEEK_END);
    program_size = ftell(fp);
    rewind(fp);
    char *program_buffer = (char*)malloc(program_size + 1);
    program_buffer[program_size] = '\0';
    fread(program_buffer, sizeof(char), program_size, fp);
    fclose(fp);
    srand(1);

    program = clCreateProgramWithSource(context, 1, (const char**)&program_buffer, &program_size, &err);
    if(err < 0) {
        perror("Couldn't create the program");
        exit(1);
    }
    free(program_buffer);

    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if(err < 0) {
        /* Find size of log and print to std output */
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
            0, NULL, &log_size);
        char *program_log = (char*) malloc(log_size + 1);
        program_log[log_size] = '\0';
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
            log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
        exit(1);
   }
    size_t compute_units = 0;
    clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);

    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(local_size), &local_size, NULL);
    if(type == 0) {
        global_size = local_size*4;
    }
    else {
        global_size = local_size * compute_units;
    }
    // Create the two input vectors
    cl_int *A = (cl_int*)malloc(sizeof(cl_int)*global_size);
    cl_int *B = (cl_int*)malloc(sizeof(cl_int)*global_size);
    cl_int *C = (cl_int*)malloc(sizeof(cl_int)*global_size);
    for(int i = 0; i < global_size; i++) {
        C[i] = 0;
        if (type == 0 || type == 1) {
            A[i] = 0;
            B[i] = 0;
        }
        else if(type == 2) {
            A[i] = rand_uint64() & 0xffffffff;
            B[i] = rand_uint64() & 0xffffffff;
        }
    }

    // Create memory buffers on the device for each vector 
    cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, 
            global_size * sizeof(cl_int), NULL, &err);
    cl_mem b_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
            global_size * sizeof(cl_int), NULL, &err);
    cl_mem c_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, 
            global_size * sizeof(cl_int), NULL, &err);

    if(err!=CL_SUCCESS){
        printf("mem allocation arguments failed");
        return 0;
    }

    queue = clCreateCommandQueueWithProperties(context, device, NULL, &err);
    if(err < 0) {
        perror("Couldn't create a command queue");
        exit(1);   
    };

    clEnqueueWriteBuffer(queue, a_mem_obj, CL_TRUE, 0,
            global_size * sizeof(cl_int), A, 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, b_mem_obj, CL_TRUE, 0, 
            global_size * sizeof(cl_int), B, 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, c_mem_obj, CL_TRUE, 0, 
            global_size * sizeof(cl_int), C, 0, NULL, NULL);

    // Set the arguments of the kernel
    kernel = clCreateKernel(program, FUNCNAME, &err);
    if(err < 0) {
        perror("Couldn't create a kernel");
        exit(1);
    };

    err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&b_mem_obj);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&c_mem_obj);
    if(err < 0) {
        perror("Couldn't create a kernel argument");
        exit(1);
    }

    cl_event kernelEvent;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, 
                &global_size, &local_size, 0, NULL, &kernelEvent);
    err |= clWaitForEvents(1, &kernelEvent);
    err |= clReleaseEvent(kernelEvent);
    err |= clFlush(queue);
    err |= clFinish(queue);
    if(err!=CL_SUCCESS){
        printf("kernel execution failed%d\n", err);
        return 0;
    }  

    // Clean up
    err = clReleaseMemObject(a_mem_obj);
    err = clReleaseMemObject(b_mem_obj);
    err = clReleaseMemObject(c_mem_obj);
    
    free(A);
    free(B);

    return 0;
}

