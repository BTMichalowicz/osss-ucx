/**
 * @file shcoll-shim.c
 * @brief Implementation of OpenSHMEM collective operations
 *
 * This file contains the implementation of OpenSHMEM collective operations
 * including:
 * - Initialization and finalization of collectives
 * - All-to-all operations (alltoall, alltoalls)
 * - Collection operations (collect, fcollect)
 * - Barrier and synchronization operations
 * - Broadcast operations
 * - Reduction operations
 */

#include "thispe.h"
#include "shmemu.h"
#include "collectives/table.h"

#include "collectives/reductions.h"
#include "collectives/typed.h"

#include "shmem/teams.h"

/**
 * @brief Helper macro to register collective operations
 * @param _cname Name of the collective operation to register
 */
#define TRY(_cname)                                                            \
  {                                                                            \
    const int s = register_##_cname(proc.env.coll._cname);                     \
                                                                               \
    if (s != 0) {                                                              \
      shmemu_fatal("couldn't register collective "                             \
                   "\"%s\" (s = %d)",                                          \
                   #_cname, s);                                                \
    }                                                                          \
  }

/**
 * @brief Initialize all collective operations
 *
 * Registers implementations for all collective operations including:
 * alltoall, alltoalls, collect, fcollect, barrier, sync, and broadcast
 */
void collectives_init(void) {
  /* Current routines */
  //   TRY(alltoall_type);
  TRY(alltoall_mem);
  TRY(alltoall_size);

  //   TRY(alltoalls_type);
  TRY(alltoalls_mem);
  TRY(alltoalls_size);

  //   TRY(collect_type);
  TRY(collect_mem);
  TRY(collect_size);

  //   TRY(fcollect_type);
  TRY(fcollect_mem);
  TRY(fcollect_size);

  //   TRY(broadcast_type);
  TRY(broadcast_mem);
  TRY(broadcast_size);

  TRY(barrier);
  TRY(barrier_all);
  TRY(sync);
  TRY(team_sync);
  TRY(sync_all);

  /* TODO: reductions */
}

/**
 * @brief Cleanup and finalize collective operations
 */
void collectives_finalize(void) { return; }

/**
 * @defgroup alltoall All-to-all Operations
 * @{
 */

// #ifdef ENABLE_PSHMEM
// #pragma weak shmem_int_alltoall = pshmem_int_alltoall
// #define shmem_int_alltoall pshmem_int_alltoall
// #pragma weak shmem_long_alltoall = pshmem_long_alltoall
// #define shmem_long_alltoall pshmem_long_alltoall
// #pragma weak shmem_longlong_alltoall = pshmem_longlong_alltoall
// #define shmem_longlong_alltoall pshmem_longlong_alltoall
// #pragma weak shmem_float_alltoall = pshmem_float_alltoall
// #define shmem_float_alltoall pshmem_float_alltoall
// #pragma weak shmem_double_alltoall = pshmem_double_alltoall
// #define shmem_double_alltoall pshmem_double_alltoall
// #pragma weak shmem_longdouble_alltoall = pshmem_longdouble_alltoall
// #define shmem_longdouble_alltoall pshmem_longdouble_alltoall
// #pragma weak shmem_uint_alltoall = pshmem_uint_alltoall
// #define shmem_uint_alltoall pshmem_uint_alltoall
// #pragma weak shmem_ulong_alltoall = pshmem_ulong_alltoall
// #define shmem_ulong_alltoall pshmem_ulong_alltoall
// #pragma weak shmem_ulonglong_alltoall = pshmem_ulonglong_alltoall
// #define shmem_ulonglong_alltoall pshmem_ulonglong_alltoall
// #pragma weak shmem_int32_alltoall = pshmem_int32_alltoall
// #define shmem_int32_alltoall pshmem_int32_alltoall
// #pragma weak shmem_int64_alltoall = pshmem_int64_alltoall
// #define shmem_int64_alltoall pshmem_int64_alltoall
// #pragma weak shmem_uint32_alltoall = pshmem_uint32_alltoall
// #define shmem_uint32_alltoall pshmem_uint32_alltoall
// #pragma weak shmem_uint64_alltoall = pshmem_uint64_alltoall
// #define shmem_uint64_alltoall pshmem_uint64_alltoall
// #pragma weak shmem_size_alltoall = pshmem_size_alltoall
// #define shmem_size_alltoall pshmem_size_alltoall
// #pragma weak shmem_ptrdiff_alltoall = pshmem_ptrdiff_alltoall
// #define shmem_ptrdiff_alltoall pshmem_ptrdiff_alltoall
// #endif /* ENABLE_PSHMEM */

// /**
//  * @brief Macro to generate typed all-to-all collective operations
//  * @param _type The C data type
//  * @param _typename The type name string
//  */
// #define SHMEM_TYPENAME_ALLTOALL(_type, _typename) \
//   int shmem_##_typename##_alltoall(shmem_team_t team, _type *dest, \
//                                    const _type *source, size_t nelems) { \
//     logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %d)", __func__, team, dest, \
//            source, nelems); \
//     colls.alltoall_type.f(team, dest, source, nelems); \
//   }

// SHMEM_TYPENAME_ALLTOALL(float, float)
// SHMEM_TYPENAME_ALLTOALL(double, double)
// SHMEM_TYPENAME_ALLTOALL(long double, longdouble)
// SHMEM_TYPENAME_ALLTOALL(char, char)
// SHMEM_TYPENAME_ALLTOALL(signed char, schar)
// SHMEM_TYPENAME_ALLTOALL(short, short)
// SHMEM_TYPENAME_ALLTOALL(int, int)
// SHMEM_TYPENAME_ALLTOALL(long, long)
// SHMEM_TYPENAME_ALLTOALL(long long, longlong)
// SHMEM_TYPENAME_ALLTOALL(unsigned char, uchar)
// SHMEM_TYPENAME_ALLTOALL(unsigned short, ushort)
// SHMEM_TYPENAME_ALLTOALL(unsigned int, uint)
// SHMEM_TYPENAME_ALLTOALL(unsigned long, ulong)
// SHMEM_TYPENAME_ALLTOALL(unsigned long long, ulonglong)
// SHMEM_TYPENAME_ALLTOALL(int8_t, int8)
// SHMEM_TYPENAME_ALLTOALL(int16_t, int16)
// SHMEM_TYPENAME_ALLTOALL(int32_t, int32)
// SHMEM_TYPENAME_ALLTOALL(int64_t, int64)
// SHMEM_TYPENAME_ALLTOALL(uint8_t, uint8)
// SHMEM_TYPENAME_ALLTOALL(uint16_t, uint16)
// SHMEM_TYPENAME_ALLTOALL(uint32_t, uint32)
// SHMEM_TYPENAME_ALLTOALL(uint64_t, uint64)
// SHMEM_TYPENAME_ALLTOALL(size_t, size)
// SHMEM_TYPENAME_ALLTOALL(ptrdiff_t, ptrdiff)

/**
 * shift_exhange_barrier (default)
 * shift_exchange_counter
 * shift_exchange_signal
 * xor_pairwise_exchange_barrier
 * xor_pairwise_exchange_counter
 * xor_pairwise_exchange_signal
 * color_pairwise_exchange_barrier
 * color_pairwise_exchange_counter
 * color_pairwise_exchange_signal
 */
SHIM_ALLTOALL_TYPE(shift_exchange_barrier)

#ifdef ENABLE_PSHMEM
#pragma weak shmem_alltoallmem = pshmem_alltoallmem
#define shmem_alltoallmem pshmem_alltoallmem
#endif /* ENABLE_PSHMEM */

/**
 * @brief Generic memory alltoall routine (deprecated)
 *
 * @param team    The team over which to alltoall
 * @param dest    Symmetric destination array on all PEs
 * @param source  Source array on root PE
 * @param nelems  Number of elements to alltoall
 * @return        Zero on success, non-zero on failure
 */
int shmem_alltoallmem(shmem_team_t team, void *dest, const void *source,
                      size_t nelems) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %d)", __func__, team, dest, source,
         nelems);
  colls.alltoall_mem.f(team, dest, source, nelems);
}

#ifdef ENABLE_PSHMEM
#pragma weak shmem_alltoall32 = pshmem_alltoall32
#define shmem_alltoall32 pshmem_alltoall32
#pragma weak shmem_alltoall64 = pshmem_alltoall64
#define shmem_alltoall64 pshmem_alltoall64
#endif /* ENABLE_PSHMEM */

/**
 * @brief Performs an all-to-all exchange of 32-bit data
 *
 * @param target Symmetric destination array
 * @param source Symmetric source array
 * @param nelems Number of elements to exchange
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
void shmem_alltoall32(void *target, const void *source, size_t nelems,
                      int PE_start, int logPE_stride, int PE_size,
                      long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %lu, %d, %d, %d, %p)", __func__, target,
         source, nelems, PE_start, logPE_stride, PE_size, pSync);

  colls.alltoall_size.f32(target, source, nelems, PE_start, logPE_stride,
                          PE_size, pSync);
}

/**
 * @brief Performs an all-to-all exchange of 64-bit data
 *
 * @param target Symmetric destination array
 * @param source Symmetric source array
 * @param nelems Number of elements to exchange
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
void shmem_alltoall64(void *target, const void *source, size_t nelems,
                      int PE_start, int logPE_stride, int PE_size,
                      long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %lu, %d, %d, %d, %p)", __func__, target,
         source, nelems, PE_start, logPE_stride, PE_size, pSync);

  colls.alltoall_size.f64(target, source, nelems, PE_start, logPE_stride,
                          PE_size, pSync);
}

/** @} */

/**
 * @defgroup alltoalls Strided All-to-all Operations
 * @{
 */

// #ifdef ENABLE_PSHMEM
// #pragma weak shmem_int_alltoalls = pshmem_int_alltoalls
// #define shmem_int_alltoalls pshmem_int_alltoalls
// #pragma weak shmem_long_alltoalls = pshmem_long_alltoalls
// #define shmem_long_alltoalls pshmem_long_alltoalls
// #pragma weak shmem_longlong_alltoalls = pshmem_longlong_alltoalls
// #define shmem_longlong_alltoalls pshmem_longlong_alltoalls
// #pragma weak shmem_float_alltoalls = pshmem_float_alltoalls
// #define shmem_float_alltoalls pshmem_float_alltoalls
// #pragma weak shmem_double_alltoalls = pshmem_double_alltoalls
// #define shmem_double_alltoalls pshmem_double_alltoalls
// #pragma weak shmem_longdouble_alltoalls = pshmem_longdouble_alltoalls
// #define shmem_longdouble_alltoalls pshmem_longdouble_alltoalls
// #pragma weak shmem_uint_alltoalls = pshmem_uint_alltoalls
// #define shmem_uint_alltoalls pshmem_uint_alltoalls
// #pragma weak shmem_ulong_alltoalls = pshmem_ulong_alltoalls
// #define shmem_ulong_alltoalls pshmem_ulong_alltoalls
// #pragma weak shmem_ulonglong_alltoalls = pshmem_ulonglong_alltoalls
// #define shmem_ulonglong_alltoalls pshmem_ulonglong_alltoalls
// #pragma weak shmem_int32_alltoalls = pshmem_int32_alltoalls
// #define shmem_int32_alltoalls pshmem_int32_alltoalls
// #pragma weak shmem_int64_alltoalls = pshmem_int64_alltoalls
// #define shmem_int64_alltoalls pshmem_int64_alltoalls
// #pragma weak shmem_uint32_alltoalls = pshmem_uint32_alltoalls
// #define shmem_uint32_alltoalls pshmem_uint32_alltoalls
// #pragma weak shmem_uint64_alltoalls = pshmem_uint64_alltoalls
// #define shmem_uint64_alltoalls pshmem_uint64_alltoalls
// #pragma weak shmem_size_alltoalls = pshmem_size_alltoalls
// #define shmem_size_alltoalls pshmem_size_alltoalls
// #pragma weak shmem_ptrdiff_alltoalls = pshmem_ptrdiff_alltoalls
// #define shmem_ptrdiff_alltoalls pshmem_ptrdiff_alltoalls
// #endif /* ENABLE_PSHMEM */

// /**
//  * @brief Macro to generate typed strided all-to-all collective operations
//  * @param _type The C data type
//  * @param _typename The type name string
//  */
// #define SHMEM_TYPENAME_ALLTOALLS(_type, _typename) \
//   int shmem_##_typename##_alltoalls(shmem_team_t team, _type *dest, \
//                                     const _type *source, ptrdiff_t dst, \
//                                     ptrdiff_t sst, size_t nelems) { \
//     logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %p, %p, %d)", __func__, team, \
//            source, dst, sst, nelems); \
//     colls.alltoalls_type.f(team, dest, source, dst, sst, nelems); \
//   }

// SHMEM_TYPENAME_ALLTOALLS(float, float)
// SHMEM_TYPENAME_ALLTOALLS(double, double)
// SHMEM_TYPENAME_ALLTOALLS(long double, longdouble)
// SHMEM_TYPENAME_ALLTOALLS(char, char)
// SHMEM_TYPENAME_ALLTOALLS(signed char, schar)
// SHMEM_TYPENAME_ALLTOALLS(short, short)
// SHMEM_TYPENAME_ALLTOALLS(int, int)
// SHMEM_TYPENAME_ALLTOALLS(long, long)
// SHMEM_TYPENAME_ALLTOALLS(long long, longlong)
// SHMEM_TYPENAME_ALLTOALLS(unsigned char, uchar)
// SHMEM_TYPENAME_ALLTOALLS(unsigned short, ushort)
// SHMEM_TYPENAME_ALLTOALLS(unsigned int, uint)
// SHMEM_TYPENAME_ALLTOALLS(unsigned long, ulong)
// SHMEM_TYPENAME_ALLTOALLS(unsigned long long, ulonglong)
// SHMEM_TYPENAME_ALLTOALLS(int8_t, int8)
// SHMEM_TYPENAME_ALLTOALLS(int16_t, int16)
// SHMEM_TYPENAME_ALLTOALLS(int32_t, int32)
// SHMEM_TYPENAME_ALLTOALLS(int64_t, int64)
// SHMEM_TYPENAME_ALLTOALLS(uint8_t, uint8)
// SHMEM_TYPENAME_ALLTOALLS(uint16_t, uint16)
// SHMEM_TYPENAME_ALLTOALLS(uint32_t, uint32)
// SHMEM_TYPENAME_ALLTOALLS(uint64_t, uint64)
// SHMEM_TYPENAME_ALLTOALLS(size_t, size)
// SHMEM_TYPENAME_ALLTOALLS(ptrdiff_t, ptrdiff)

/**
 * shift_exchange_barrier (default)
 * shift_exchange_counter
 * xor_pairwise_exchange_barrier
 * xor_pairwise_exchange_counter
 * color_pairwise_exchange_barrier
 * color_pairwise_exchange_counter
 */
SHIM_ALLTOALLS_TYPE(shift_exchange_barrier)

#ifdef ENABLE_PSHMEM
#pragma weak shmem_alltoallsmem = pshmem_alltoallsmem
#define shmem_alltoallsmem pshmem_alltoallsmem
#endif /* ENABLE_PSHMEM */

/**
 * @brief Generic memory alltoall routine (deprecated)
 *
 * @param team    The team over which to alltoall
 * @param dest    Symmetric destination array on all PEs
 * @param source  Source array on root PE
 * @param dst     Destination array on root PE
 * @param sst     Source array on root PE
 * @param nelems  Number of elements to alltoall
 * @return        Zero on success, non-zero on failure
 */
int shmem_alltoallsmem(shmem_team_t team, void *dest, const void *source,
                       ptrdiff_t dst, ptrdiff_t sst, size_t nelems) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %td, %td, %d)", __func__, team, dest,
         source, dst, sst, nelems);
  colls.alltoalls_mem.f(team, dest, source, dst, sst, nelems);
}

#ifdef ENABLE_PSHMEM
#pragma weak shmem_alltoalls32 = pshmem_alltoalls32
#define shmem_alltoalls32 pshmem_alltoalls32
#pragma weak shmem_alltoalls64 = pshmem_alltoalls64
#define shmem_alltoalls64 pshmem_alltoalls64
#endif /* ENABLE_PSHMEM */

/**
 * @brief Performs a strided all-to-all exchange of 32-bit data
 *
 * @param target Symmetric destination array
 * @param source Symmetric source array
 * @param dst Target array element stride
 * @param sst Source array element stride
 * @param nelems Number of elements to exchange
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
void shmem_alltoalls32(void *target, const void *source, ptrdiff_t dst,
                       ptrdiff_t sst, size_t nelems, int PE_start,
                       int logPE_stride, int PE_size, long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %lu, %lu, %lu, %d, %d, %d, %p)", __func__,
         target, source, dst, sst, nelems, PE_start, logPE_stride, PE_size,
         pSync);

  colls.alltoalls_size.f32(target, source, dst, sst, nelems, PE_start,
                           logPE_stride, PE_size, pSync);
}

/**
 * @brief Performs a strided all-to-all exchange of 64-bit data
 *
 * @param target Symmetric destination array
 * @param source Symmetric source array
 * @param dst Target array element stride
 * @param sst Source array element stride
 * @param nelems Number of elements to exchange
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
void shmem_alltoalls64(void *target, const void *source, ptrdiff_t dst,
                       ptrdiff_t sst, size_t nelems, int PE_start,
                       int logPE_stride, int PE_size, long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %lu, %lu, %lu, %d, %d, %d, %p)", __func__,
         target, source, dst, sst, nelems, PE_start, logPE_stride, PE_size,
         pSync);

  colls.alltoalls_size.f64(target, source, dst, sst, nelems, PE_start,
                           logPE_stride, PE_size, pSync);
}

/** @} */

/**
 * @defgroup collect Collection Operations
 * @{
 */

// #ifdef ENABLE_PSHMEM
// #pragma weak shmem_float_collect = pshmem_float_collect
// #define shmem_float_collect pshmem_float_collect
// #pragma weak shmem_double_collect = pshmem_double_collect
// #define shmem_double_collect pshmem_double_collect
// #pragma weak shmem_longdouble_collect = pshmem_longdouble_collect
// #define shmem_longdouble_collect pshmem_longdouble_collect
// #pragma weak shmem_char_collect = pshmem_char_collect
// #define shmem_char_collect pshmem_char_collect
// #pragma weak shmem_schar_collect = pshmem_schar_collect
// #define shmem_schar_collect pshmem_schar_collect
// #pragma weak shmem_short_collect = pshmem_short_collect
// #define shmem_short_collect pshmem_short_collect
// #pragma weak shmem_int_collect = pshmem_int_collect
// #define shmem_int_collect pshmem_int_collect
// #pragma weak shmem_long_collect = pshmem_long_collect
// #define shmem_long_collect pshmem_long_collect
// #pragma weak shmem_longlong_collect = pshmem_longlong_collect
// #define shmem_longlong_collect pshmem_longlong_collect
// #pragma weak shmem_uchar_collect = pshmem_uchar_collect
// #define shmem_uchar_collect pshmem_uchar_collect
// #pragma weak shmem_ushort_collect = pshmem_ushort_collect
// #define shmem_ushort_collect pshmem_ushort_collect
// #pragma weak shmem_uint_collect = pshmem_uint_collect
// #define shmem_uint_collect pshmem_uint_collect
// #pragma weak shmem_ulong_collect = pshmem_ulong_collect
// #define shmem_ulong_collect pshmem_ulong_collect
// #pragma weak shmem_ulonglong_collect = pshmem_ulonglong_collect
// #define shmem_ulonglong_collect pshmem_ulonglong_collect
// #pragma weak shmem_int8_collect = pshmem_int8_collect
// #define shmem_int8_collect pshmem_int8_collect
// #pragma weak shmem_int16_collect = pshmem_int16_collect
// #define shmem_int16_collect pshmem_int16_collect
// #pragma weak shmem_int32_collect = pshmem_int32_collect
// #define shmem_int32_collect pshmem_int32_collect
// #pragma weak shmem_int64_collect = pshmem_int64_collect
// #define shmem_int64_collect pshmem_int64_collect
// #pragma weak shmem_uint8_collect = pshmem_uint8_collect
// #define shmem_uint8_collect pshmem_uint8_collect
// #pragma weak shmem_uint16_collect = pshmem_uint16_collect
// #define shmem_uint16_collect pshmem_uint16_collect
// #pragma weak shmem_uint32_collect = pshmem_uint32_collect
// #define shmem_uint32_collect pshmem_uint32_collect
// #pragma weak shmem_uint64_collect = pshmem_uint64_collect
// #define shmem_uint64_collect pshmem_uint64_collect
// #pragma weak shmem_size_collect = pshmem_size_collect
// #define shmem_size_collect pshmem_size_collect
// #pragma weak shmem_ptrdiff_collect = pshmem_ptrdiff_collect
// #define shmem_ptrdiff_collect pshmem_ptrdiff_collect
// #endif /* ENABLE_PSHMEM */

// /**
//  * @brief Macro to generate typed collect operations
//  * @param _type The C data type
//  * @param _typename The type name string
//  */
// #define SHMEM_TYPENAME_COLLECT(_type, _typename) \
//   int shmem_##_typename##_collect(shmem_team_t team, _type *dest, \
//                                   const _type *source, size_t nelems) { \
//     logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %d)", __func__, dest, source, \
//            nelems); \
//     colls.collect_type.f(team, dest, source, nelems); \
//   }

// SHMEM_TYPENAME_COLLECT(float, float)
// SHMEM_TYPENAME_COLLECT(double, double)
// SHMEM_TYPENAME_COLLECT(long double, longdouble)
// SHMEM_TYPENAME_COLLECT(char, char)
// SHMEM_TYPENAME_COLLECT(signed char, schar)
// SHMEM_TYPENAME_COLLECT(short, short)
// SHMEM_TYPENAME_COLLECT(int, int)
// SHMEM_TYPENAME_COLLECT(long, long)
// SHMEM_TYPENAME_COLLECT(long long, longlong)
// SHMEM_TYPENAME_COLLECT(unsigned char, uchar)
// SHMEM_TYPENAME_COLLECT(unsigned short, ushort)
// SHMEM_TYPENAME_COLLECT(unsigned int, uint)
// SHMEM_TYPENAME_COLLECT(unsigned long, ulong)
// SHMEM_TYPENAME_COLLECT(unsigned long long, ulonglong)
// SHMEM_TYPENAME_COLLECT(int8_t, int8)
// SHMEM_TYPENAME_COLLECT(int16_t, int16)
// SHMEM_TYPENAME_COLLECT(int32_t, int32)
// SHMEM_TYPENAME_COLLECT(int64_t, int64)
// SHMEM_TYPENAME_COLLECT(uint8_t, uint8)
// SHMEM_TYPENAME_COLLECT(uint16_t, uint16)
// SHMEM_TYPENAME_COLLECT(uint32_t, uint32)
// SHMEM_TYPENAME_COLLECT(uint64_t, uint64)
// SHMEM_TYPENAME_COLLECT(size_t, size)
// SHMEM_TYPENAME_COLLECT(ptrdiff_t, ptrdiff)

/**
 * bruck (default)
 * bruck_no_rotate
 * linear
 * all_linear
 * all_linear1
 * rec_dbl
 * rec_dbl_signal
 * ring
 * simple
 */
SHIM_COLLECT_TYPE(bruck)

#ifdef ENABLE_PSHMEM
#pragma weak shmem_collectmem = pshmem_collectmem
#define shmem_collectmem pshmem_collectmem
#endif /* ENABLE_PSHMEM */

/**
 * @brief Generic memory collect routine (deprecated)
 *
 * @param team    The team over which to collect
 * @param dest    Symmetric destination array on all PEs
 * @param source  Source array on root PE
 * @param nelems  Number of elements to collect
 * @return        Zero on success, non-zero on failure
 */
int shmem_collectmem(shmem_team_t team, void *dest, const void *source,
                     size_t nelems) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %d)", __func__, team, dest, source,
         nelems);
  colls.collect_mem.f(team, dest, source, nelems);
}

#ifdef ENABLE_PSHMEM
#pragma weak shmem_collect32 = pshmem_collect32
#define shmem_collect32 pshmem_collect32
#pragma weak shmem_collect64 = pshmem_collect64
#define shmem_collect64 pshmem_collect64
#endif /* ENABLE_PSHMEM */

/**
 * @brief Concatenates 32-bit data from multiple PEs to an array in ascending PE
 * order
 *
 * @param target Symmetric destination array
 * @param source Symmetric source array
 * @param nelems Number of elements to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
void shmem_collect32(void *target, const void *source, size_t nelems,
                     int PE_start, int logPE_stride, int PE_size, long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %lu, %d, %d, %d, %p)", __func__, target,
         source, nelems, PE_start, logPE_stride, PE_size, pSync);

  colls.collect_size.f32(target, source, nelems, PE_start, logPE_stride,
                         PE_size, pSync);
}

/**
 * @brief Concatenates 64-bit data from multiple PEs to an array in ascending PE
 * order
 *
 * @param target Symmetric destination array
 * @param source Symmetric source array
 * @param nelems Number of elements to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
void shmem_collect64(void *target, const void *source, size_t nelems,
                     int PE_start, int logPE_stride, int PE_size, long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %lu, %d, %d, %d, %p)", __func__, target,
         source, nelems, PE_start, logPE_stride, PE_size, pSync);

  colls.collect_size.f64(target, source, nelems, PE_start, logPE_stride,
                         PE_size, pSync);
}

/** @} */

/**
 * @defgroup fcollect Fixed-Length Collection Operations
 * @{
 */

// #ifdef ENABLE_PSHMEM
// #pragma weak shmem_float_fcollect = pshmem_float_fcollect
// #define shmem_float_fcollect pshmem_float_fcollect
// #pragma weak shmem_double_fcollect = pshmem_double_fcollect
// #define shmem_double_fcollect pshmem_double_fcollect
// #pragma weak shmem_longdouble_fcollect = pshmem_longdouble_fcollect
// #define shmem_longdouble_fcollect pshmem_longdouble_fcollect
// #pragma weak shmem_char_fcollect = pshmem_char_fcollect
// #define shmem_char_fcollect pshmem_char_fcollect
// #pragma weak shmem_schar_fcollect = pshmem_schar_fcollect
// #define shmem_schar_fcollect pshmem_schar_fcollect
// #pragma weak shmem_short_fcollect = pshmem_short_fcollect
// #define shmem_short_fcollect pshmem_short_fcollect
// #pragma weak shmem_int_fcollect = pshmem_int_fcollect
// #define shmem_int_fcollect pshmem_int_fcollect
// #pragma weak shmem_long_fcollect = pshmem_long_fcollect
// #define shmem_long_fcollect pshmem_long_fcollect
// #pragma weak shmem_longlong_fcollect = pshmem_longlong_fcollect
// #define shmem_longlong_fcollect pshmem_longlong_fcollect
// #pragma weak shmem_uchar_fcollect = pshmem_uchar_fcollect
// #define shmem_uchar_fcollect pshmem_uchar_fcollect
// #pragma weak shmem_ushort_fcollect = pshmem_ushort_fcollect
// #define shmem_ushort_fcollect pshmem_ushort_fcollect
// #pragma weak shmem_uint_fcollect = pshmem_uint_fcollect
// #define shmem_uint_fcollect pshmem_uint_fcollect
// #pragma weak shmem_ulong_fcollect = pshmem_ulong_fcollect
// #define shmem_ulong_fcollect pshmem_ulong_fcollect
// #pragma weak shmem_ulonglong_fcollect = pshmem_ulonglong_fcollect
// #define shmem_ulonglong_fcollect pshmem_ulonglong_fcollect
// #pragma weak shmem_int8_fcollect = pshmem_int8_fcollect
// #define shmem_int8_fcollect pshmem_int8_fcollect
// #pragma weak shmem_int16_fcollect = pshmem_int16_fcollect
// #define shmem_int16_fcollect pshmem_int16_fcollect
// #pragma weak shmem_int32_fcollect = pshmem_int32_fcollect
// #define shmem_int32_fcollect pshmem_int32_fcollect
// #pragma weak shmem_int64_fcollect = pshmem_int64_fcollect
// #define shmem_int64_fcollect pshmem_int64_fcollect
// #pragma weak shmem_uint8_fcollect = pshmem_uint8_fcollect
// #define shmem_uint8_fcollect pshmem_uint8_fcollect
// #pragma weak shmem_uint16_fcollect = pshmem_uint16_fcollect
// #define shmem_uint16_fcollect pshmem_uint16_fcollect
// #pragma weak shmem_uint32_fcollect = pshmem_uint32_fcollect
// #define shmem_uint32_fcollect pshmem_uint32_fcollect
// #pragma weak shmem_uint64_fcollect = pshmem_uint64_fcollect
// #define shmem_uint64_fcollect pshmem_uint64_fcollect
// #pragma weak shmem_size_fcollect = pshmem_size_fcollect
// #define shmem_size_fcollect pshmem_size_fcollect
// #pragma weak shmem_ptrdiff_fcollect = pshmem_ptrdiff_fcollect
// #define shmem_ptrdiff_fcollect pshmem_ptrdiff_fcollect
// #endif /* ENABLE_PSHMEM */

// /**
//  * @brief Macro to generate typed fixed-length collect operations
//  * @param _type The C data type
//  * @param _typename The type name string
//  */
// #define SHMEM_TYPENAME_FCOLLECT(_type, _typename) \
//   int shmem_##_typename##_fcollect(shmem_team_t team, _type *dest, \
//                                    const _type *source, size_t nelems) { \
//     logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %d)", __func__, dest, source, \
//            nelems); \
//     colls.fcollect_type.f(team, dest, source, nelems); \
//   }

// SHMEM_TYPENAME_FCOLLECT(float, float)
// SHMEM_TYPENAME_FCOLLECT(double, double)
// SHMEM_TYPENAME_FCOLLECT(long double, longdouble)
// SHMEM_TYPENAME_FCOLLECT(char, char)
// SHMEM_TYPENAME_FCOLLECT(signed char, schar)
// SHMEM_TYPENAME_FCOLLECT(short, short)
// SHMEM_TYPENAME_FCOLLECT(int, int)
// SHMEM_TYPENAME_FCOLLECT(long, long)
// SHMEM_TYPENAME_FCOLLECT(long long, longlong)
// SHMEM_TYPENAME_FCOLLECT(unsigned char, uchar)
// SHMEM_TYPENAME_FCOLLECT(unsigned short, ushort)
// SHMEM_TYPENAME_FCOLLECT(unsigned int, uint)
// SHMEM_TYPENAME_FCOLLECT(unsigned long, ulong)
// SHMEM_TYPENAME_FCOLLECT(unsigned long long, ulonglong)
// SHMEM_TYPENAME_FCOLLECT(int8_t, int8)
// SHMEM_TYPENAME_FCOLLECT(int16_t, int16)
// SHMEM_TYPENAME_FCOLLECT(int32_t, int32)
// SHMEM_TYPENAME_FCOLLECT(int64_t, int64)
// SHMEM_TYPENAME_FCOLLECT(uint8_t, uint8)
// SHMEM_TYPENAME_FCOLLECT(uint16_t, uint16)
// SHMEM_TYPENAME_FCOLLECT(uint32_t, uint32)
// SHMEM_TYPENAME_FCOLLECT(uint64_t, uint64)
// SHMEM_TYPENAME_FCOLLECT(size_t, size)
// SHMEM_TYPENAME_FCOLLECT(ptrdiff_t, ptrdiff)

/**
 * bruck_inplace (default)
 * bruck
 * bruck_no_rotate
 * bruck_signal
 * linear
 * all_linear
 * all_linear1
 * rec_dbl
 * ring
 * neighbor_exchange
 */
SHIM_FCOLLECT_TYPE(bruck_inplace)

#ifdef ENABLE_PSHMEM
#pragma weak shmem_fcollectmem = pshmem_fcollectmem
#define shmem_fcollectmem pshmem_fcollectmem
#endif /* ENABLE_PSHMEM */

/**
 * @brief Generic memory collect routine (deprecated)
 *
 * @param team    The team over which to collect
 * @param dest    Symmetric destination array on all PEs
 * @param source  Source array on root PE
 * @param nelems  Number of elements to collect
 * @return        Zero on success, non-zero on failure
 */
int shmem_fcollectmem(shmem_team_t team, void *dest, const void *source,
                      size_t nelems) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %d)", __func__, team, dest, source,
         nelems);
  colls.fcollect_mem.f(team, dest, source, nelems);
}

#ifdef ENABLE_PSHMEM
#pragma weak shmem_fcollect32 = pshmem_fcollect32
#define shmem_fcollect32 pshmem_fcollect32
#pragma weak shmem_fcollect64 = pshmem_fcollect64
#define shmem_fcollect64 pshmem_fcollect64
#endif /* ENABLE_PSHMEM */

/**
 * @brief Concatenates fixed-length 32-bit data from multiple PEs to an array in
 * ascending PE order
 *
 * @param target Symmetric destination array
 * @param source Symmetric source array
 * @param nelems Number of elements to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
void shmem_fcollect32(void *target, const void *source, size_t nelems,
                      int PE_start, int logPE_stride, int PE_size,
                      long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %lu, %d, %d, %d, %p)", __func__, target,
         source, nelems, PE_start, logPE_stride, PE_size, pSync);

  colls.fcollect_size.f32(target, source, nelems, PE_start, logPE_stride,
                          PE_size, pSync);
}

/**
 * @brief Concatenates fixed-length 64-bit data from multiple PEs to an array in
 * ascending PE order
 *
 * @param target Symmetric destination array
 * @param source Symmetric source array
 * @param nelems Number of elements to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
void shmem_fcollect64(void *target, const void *source, size_t nelems,
                      int PE_start, int logPE_stride, int PE_size,
                      long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %lu, %d, %d, %d, %p)", __func__, target,
         source, nelems, PE_start, logPE_stride, PE_size, pSync);

  colls.fcollect_size.f64(target, source, nelems, PE_start, logPE_stride,
                          PE_size, pSync);
}

/** @} */

/**
 * @defgroup Broadcast Operations
 * @{
 */

// #ifdef ENABLE_PSHMEM
// #pragma weak shmem_float_broadcast = pshmem_float_broadcast
// #define shmem_float_broadcast pshmem_float_broadcast
// #pragma weak shmem_double_broadcast = pshmem_double_broadcast
// #define shmem_double_broadcast pshmem_double_broadcast
// #pragma weak shmem_longdouble_broadcast = pshmem_longdouble_broadcast
// #define shmem_longdouble_broadcast pshmem_longdouble_broadcast
// #pragma weak shmem_char_broadcast = pshmem_char_broadcast
// #define shmem_char_broadcast pshmem_char_broadcast
// #pragma weak shmem_schar_broadcast = pshmem_schar_broadcast
// #define shmem_schar_broadcast pshmem_schar_broadcast
// #pragma weak shmem_short_broadcast = pshmem_short_broadcast
// #define shmem_short_broadcast pshmem_short_broadcast
// #pragma weak shmem_int_broadcast = pshmem_int_broadcast
// #define shmem_int_broadcast pshmem_int_broadcast
// #pragma weak shmem_long_broadcast = pshmem_long_broadcast
// #define shmem_long_broadcast pshmem_long_broadcast
// #pragma weak shmem_longlong_broadcast = pshmem_longlong_broadcast
// #define shmem_longlong_broadcast pshmem_longlong_broadcast
// #pragma weak shmem_uchar_broadcast = pshmem_uchar_broadcast
// #define shmem_uchar_broadcast pshmem_uchar_broadcast
// #pragma weak shmem_ushort_broadcast = pshmem_ushort_broadcast
// #define shmem_ushort_broadcast pshmem_ushort_broadcast
// #pragma weak shmem_uint_broadcast = pshmem_uint_broadcast
// #define shmem_uint_broadcast pshmem_uint_broadcast
// #pragma weak shmem_ulong_broadcast = pshmem_ulong_broadcast
// #define shmem_ulong_broadcast pshmem_ulong_broadcast
// #pragma weak shmem_ulonglong_broadcast = pshmem_ulonglong_broadcast
// #define shmem_ulonglong_broadcast pshmem_ulonglong_broadcast
// #pragma weak shmem_int8_broadcast = pshmem_int8_broadcast
// #define shmem_int8_broadcast pshmem_int8_broadcast
// #pragma weak shmem_int16_broadcast = pshmem_int16_broadcast
// #define shmem_int16_broadcast pshmem_int16_broadcast
// #pragma weak shmem_int32_broadcast = pshmem_int32_broadcast
// #define shmem_int32_broadcast pshmem_int32_broadcast
// #pragma weak shmem_int64_broadcast = pshmem_int64_broadcast
// #define shmem_int64_broadcast pshmem_int64_broadcast
// #pragma weak shmem_uint8_broadcast = pshmem_uint8_broadcast
// #define shmem_uint8_broadcast pshmem_uint8_broadcast
// #pragma weak shmem_uint16_broadcast = pshmem_uint16_broadcast
// #define shmem_uint16_broadcast pshmem_uint16_broadcast
// #pragma weak shmem_uint32_broadcast = pshmem_uint32_broadcast
// #define shmem_uint32_broadcast pshmem_uint32_broadcast
// #pragma weak shmem_uint64_broadcast = pshmem_uint64_broadcast
// #define shmem_uint64_broadcast pshmem_uint64_broadcast
// #pragma weak shmem_size_broadcast = pshmem_size_broadcast
// #define shmem_size_broadcast pshmem_size_broadcast
// #pragma weak shmem_ptrdiff_broadcast = pshmem_ptrdiff_broadcast
// #define shmem_ptrdiff_broadcast pshmem_ptrdiff_broadcast
// #endif /* ENABLE_PSHMEM */

// /**
//  * @brief Macro to generate typed fixed-length broadcast operations
//  * @param _type The C data type
//  * @param _typename The type name string
//  */
// #define SHMEM_TYPENAME_BROADCAST(_type, _typename) \
//   int shmem_##_typename##_broadcast(shmem_team_t team, _type *dest, \
//                                     const _type *source, size_t nelems, \
//                                     int PE_root) { \
//     logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %d, %d)", __func__, dest, source,
//     \
//            nelems, PE_root); \
//     colls.broadcast_type.f(team, dest, source, nelems, PE_root); \
//   }

// SHMEM_TYPENAME_BROADCAST(float, float)
// SHMEM_TYPENAME_BROADCAST(double, double)
// SHMEM_TYPENAME_BROADCAST(long double, longdouble)
// SHMEM_TYPENAME_BROADCAST(char, char)
// SHMEM_TYPENAME_BROADCAST(signed char, schar)
// SHMEM_TYPENAME_BROADCAST(short, short)
// SHMEM_TYPENAME_BROADCAST(int, int)
// SHMEM_TYPENAME_BROADCAST(long, long)
// SHMEM_TYPENAME_BROADCAST(long long, longlong)
// SHMEM_TYPENAME_BROADCAST(unsigned char, uchar)
// SHMEM_TYPENAME_BROADCAST(unsigned short, ushort)
// SHMEM_TYPENAME_BROADCAST(unsigned int, uint)
// SHMEM_TYPENAME_BROADCAST(unsigned long, ulong)
// SHMEM_TYPENAME_BROADCAST(unsigned long long, ulonglong)
// SHMEM_TYPENAME_BROADCAST(int8_t, int8)
// SHMEM_TYPENAME_BROADCAST(int16_t, int16)
// SHMEM_TYPENAME_BROADCAST(int32_t, int32)
// SHMEM_TYPENAME_BROADCAST(int64_t, int64)
// SHMEM_TYPENAME_BROADCAST(uint8_t, uint8)
// SHMEM_TYPENAME_BROADCAST(uint16_t, uint16)
// SHMEM_TYPENAME_BROADCAST(uint32_t, uint32)
// SHMEM_TYPENAME_BROADCAST(uint64_t, uint64)
// SHMEM_TYPENAME_BROADCAST(size_t, size)
// SHMEM_TYPENAME_BROADCAST(ptrdiff_t, ptrdiff)

/**
 * binomial_tree (default)
 * complete_tree
 * linear
 * scatter_collect
 * knomial_tree
 * knomial_tree_signal
 */
SHIM_BROADCAST_TYPE(binomial_tree)

#ifdef ENABLE_PSHMEM
#pragma weak shmem_broadcastmem = pshmem_broadcastmem
#define shmem_broadcastmem pshmem_broadcastmem
#endif /* ENABLE_PSHMEM */

/**
 * @brief Generic memory broadcast routine (deprecated)
 *
 * @param team    The team over which to broadcast
 * @param dest    Symmetric destination array on all PEs
 * @param source  Source array on root PE
 * @param nelems  Number of elements to broadcast
 * @param PE_root The root PE
 * @return        Zero on success, non-zero on failure
 */
int shmem_broadcastmem(shmem_team_t team, void *dest, const void *source,
                       size_t nelems, int PE_root) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %d, %d)", __func__, dest, source,
         nelems, PE_root);
  colls.broadcast_mem.f(team, dest, source, nelems, PE_root);
}

#ifdef ENABLE_PSHMEM
#pragma weak shmem_broadcast32 = pshmem_broadcast32
#define shmem_broadcast32 pshmem_broadcast32
#pragma weak shmem_broadcast64 = pshmem_broadcast64
#define shmem_broadcast64 pshmem_broadcast64
#endif /* ENABLE_PSHMEM */

/**
 * @brief Broadcasts 32-bit data from a source PE to all other PEs in a group
 *
 * @param target Symmetric destination array
 * @param source Symmetric source array
 * @param nelems Number of elements to broadcast
 * @param PE_root Source PE for the broadcast
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
void shmem_broadcast32(void *target, const void *source, size_t nelems,
                       int PE_root, int PE_start, int logPE_stride, int PE_size,
                       long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %lu, %d, %d, %d, %p)", __func__, target,
         source, nelems, PE_start, logPE_stride, PE_size, pSync);

  colls.broadcast_size.f32(target, source, nelems, PE_root, PE_start,
                           logPE_stride, PE_size, pSync);
}

/**
 * @brief Broadcasts 64-bit data from a source PE to all other PEs in a group
 *
 * @param target Symmetric destination array
 * @param source Symmetric source array
 * @param nelems Number of elements to broadcast
 * @param PE_root Source PE for the broadcast
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
void shmem_broadcast64(void *target, const void *source, size_t nelems,
                       int PE_root, int PE_start, int logPE_stride, int PE_size,
                       long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %lu, %d, %d, %d, %p)", __func__, target,
         source, nelems, PE_start, logPE_stride, PE_size, pSync);

  colls.broadcast_size.f64(target, source, nelems, PE_root, PE_start,
                           logPE_stride, PE_size, pSync);
}

/** @} */
///////////////////////////////////////////////////////////////////////

/**
 * @defgroup TODO: reductions Reduction Operations
 * @{
 */

/*
 * reductions:
 *
 * linear
 * binomial
 * rec_dbl
 * rabenseifner
 * rabenseifner2
 *
 */
SHIM_TO_ALL_ALL(rec_dbl)
SHIM_REDUCE_ALL(rec_dbl)
/** @} */

/** @} */

///////////////////////////////////////////////////////////////////////
/**
 * @defgroup barrier Barrier Operations
 * @{
 */

/*
 * sync variables supplied by me
 */
extern long *shmemc_barrier_all_psync;
extern long *shmemc_sync_all_psync;

#ifdef ENABLE_PSHMEM
#pragma weak shmem_barrier_all = pshmem_barrier_all
#define shmem_barrier_all pshmem_barrier_all
#endif /* ENABLE_PSHMEM */

/**
 * @brief Barrier synchronization across all PEs
 */
void shmem_barrier_all(void) {
  logger(LOG_COLLECTIVES, "%s()", __func__);

  colls.barrier_all.f(shmemc_barrier_all_psync);
}

/** @} */

///////////////////////////////////////////////////////////////////////

#ifdef ENABLE_PSHMEM
#pragma weak shmem_sync_all = pshmem_sync_all
#define shmem_sync_all pshmem_sync_all
#endif /* ENABLE_PSHMEM */

/**
 * @brief Synchronize across all PEs
 */
void shmem_sync_all(void) {
  logger(LOG_COLLECTIVES, "%s()", __func__);

  colls.sync_all.f(shmemc_sync_all_psync);
}

/** @} */
///////////////////////////////////////////////////////////////////////

/**
 * @defgroup sync Deprecated Synchronization Operations
 * @{
 */

#ifdef ENABLE_PSHMEM
#pragma weak shmem_sync = pshmem_sync
#define shmem_sync pshmem_sync
#endif /* ENABLE_PSHMEM */

/**
 * @brief Synchronizes a subset of PEs
 *
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
void shmem_sync_deprecated(int PE_start, int logPE_stride, int PE_size, long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%d, %d, %d, %p)", __func__, PE_start,
         logPE_stride, PE_size, pSync);
  colls.sync.f(PE_start, logPE_stride, PE_size, pSync);
}

#ifdef ENABLE_PSHMEM
#pragma weak shmem_barrier = pshmem_barrier
#define shmem_barrier pshmem_barrier
#endif /* ENABLE_PSHMEM */

/**
 * @brief Performs a barrier synchronization across a subset of PEs
 *
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
void shmem_barrier(int PE_start, int logPE_stride, int PE_size, long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%d, %d, %d, %p)", __func__, PE_start,
         logPE_stride, PE_size, pSync);

  colls.barrier.f(PE_start, logPE_stride, PE_size, pSync);
}

#ifdef ENABLE_PSHMEM
#pragma weak shmem_team_sync = pshmem_team_sync
#define shmem_team_sync pshmem_team_sync
#endif /* ENABLE_PSHMEM */

/**
 * @brief Synchronizes a team of PEs
 *
 * @param team The team to synchronize
 */
int shmem_team_sync(shmem_team_t team) {
  logger(LOG_COLLECTIVES, "%s(%p)", __func__, team);

  colls.team_sync.f(team);
}

/** @} */
