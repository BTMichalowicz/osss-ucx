#include "../build/install/include/shmem.h"
#include <stdio.h>
#include <inttypes.h>




int main(void) {
    int my_pe, npes;

    /* Initialize OpenSHMEM */
    shmem_init();

    my_pe = shmem_my_pe();
    npes = shmem_n_pes();

    /* Ensure there are at least two PEs */
    if (npes < 2) {
        if (my_pe == 0) {
            printf("This program requires at least two PEs.\n");
        }
        shmem_finalize();
        return 1;
    }

    // Macro for testing put for each type
    #define TEST_PUT(TYPE, TYPENAME, FMT) \
        TYPE *src_##TYPENAME = shmem_malloc(10 * sizeof(TYPE)); \
        TYPE *dest_##TYPENAME = shmem_malloc(10 * sizeof(TYPE)); \
        for (int i = 0; i < 10; i++) src_##TYPENAME[i] = (TYPE)(i + 1); \
        shmem_barrier_all(); \
        if (my_pe == 0) shmem_##TYPENAME##_put(dest_##TYPENAME, src_##TYPENAME, 10, 1); \
        shmem_barrier_all(); \
        if (my_pe == 1) { \
            printf("PE %d: " #TYPENAME " received: ", my_pe); \
            for (int i = 0; i < 10; i++) printf(FMT " ", dest_##TYPENAME[i]); \
            printf("\n"); \
        } \
        shmem_free(src_##TYPENAME); \
        shmem_free(dest_##TYPENAME);

    // Test all types in SHMEM_STANDARD_RMA_TYPE_TABLE
    TEST_PUT(float, float, "%f");
    TEST_PUT(double, double, "%lf");
    TEST_PUT(long double, longdouble, "%Lf");
    TEST_PUT(char, char, "%d");
    TEST_PUT(signed char, schar, "%d");
    TEST_PUT(short, short, "%d");
    TEST_PUT(int, int, "%d");
    TEST_PUT(long, long, "%ld");
    TEST_PUT(long long, longlong, "%lld");
    TEST_PUT(unsigned char, uchar, "%u");
    TEST_PUT(unsigned short, ushort, "%u");
    TEST_PUT(unsigned int, uint, "%u");
    TEST_PUT(unsigned long, ulong, "%lu");
    TEST_PUT(unsigned long long, ulonglong, "%llu");
    TEST_PUT(int8_t, int8, "%" PRId8);
    TEST_PUT(int16_t, int16, "%" PRId16);
    TEST_PUT(int32_t, int32, "%" PRId32);
    TEST_PUT(int64_t, int64, "%" PRId64);
    TEST_PUT(uint8_t, uint8, "%" PRIu8);
    TEST_PUT(uint16_t, uint16, "%" PRIu16);
    TEST_PUT(uint32_t, uint32, "%" PRIu32);
    TEST_PUT(uint64_t, uint64, "%" PRIu64);
    TEST_PUT(size_t, size, "%zu");
    TEST_PUT(ptrdiff_t, ptrdiff, "%td");

    #undef TEST_PUT

    /* Finalize OpenSHMEM */
    shmem_finalize();

    return 0;
}
