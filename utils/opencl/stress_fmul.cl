__kernel void stress(__global const float *A, __global const float *B, __global float *C) {
 
    int i = get_global_id(0);

    float a = A[i];
    float b = B[i];
    float c = C[i];
    for(ulong count=0; count < 4000000000; count++) {
        c *= a;
    }
    for(ulong count=0; count < 4000000000; count++) {
        c *= a;
    }
    for(ulong count=0; count < 4000000000; count++) {
        c *= a;
    }
    for(ulong count=0; count < 4000000000; count++) {
        c *= a;
    }
    C[i] = c;
}