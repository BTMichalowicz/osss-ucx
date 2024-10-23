/* For license: see LICENSE file at top-level */

#include "thispe.h"
#include "shmemu.h"
#include "collectives/table.h"

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

void collectives_init(void) {
  TRY(alltoall);
  TRY(alltoalls);
  TRY(collect);
  TRY(fcollect);
  TRY(barrier);
  TRY(barrier_all);
  TRY(sync);
  TRY(sync_all);
  TRY(broadcast);

  /* TODO: reductions */
}

void collectives_finalize(void) { return; }

/**
 * shmem_alltoall
 */
#ifdef ENABLE_PSHMEM
#pragma weak shmem_int_alltoall = pshmem_int_alltoall
#define shmem_int_alltoall pshmem_int_alltoall
#pragma weak shmem_long_alltoall = pshmem_long_alltoall
#define shmem_long_alltoall pshmem_long_alltoall
#pragma weak shmem_longlong_alltoall = pshmem_longlong_alltoall
#define shmem_longlong_alltoall pshmem_longlong_alltoall
#pragma weak shmem_float_alltoall = pshmem_float_alltoall
#define shmem_float_alltoall pshmem_float_alltoall
#pragma weak shmem_double_alltoall = pshmem_double_alltoall
#define shmem_double_alltoall pshmem_double_alltoall
#pragma weak shmem_longdouble_alltoall = pshmem_longdouble_alltoall
#define shmem_longdouble_alltoall pshmem_longdouble_alltoall
#pragma weak shmem_uint_alltoall = pshmem_uint_alltoall
#define shmem_uint_alltoall pshmem_uint_alltoall
#pragma weak shmem_ulong_alltoall = pshmem_ulong_alltoall
#define shmem_ulong_alltoall pshmem_ulong_alltoall
#pragma weak shmem_ulonglong_alltoall = pshmem_ulonglong_alltoall
#define shmem_ulonglong_alltoall pshmem_ulonglong_alltoall
#pragma weak shmem_int32_alltoall = pshmem_int32_alltoall
#define shmem_int32_alltoall pshmem_int32_alltoall
#pragma weak shmem_int64_alltoall = pshmem_int64_alltoall
#define shmem_int64_alltoall pshmem_int64_alltoall
#pragma weak shmem_uint32_alltoall = pshmem_uint32_alltoall
#define shmem_uint32_alltoall pshmem_uint32_alltoall
#pragma weak shmem_uint64_alltoall = pshmem_uint64_alltoall
#define shmem_uint64_alltoall pshmem_uint64_alltoall
#pragma weak shmem_size_alltoall = pshmem_size_alltoall
#define shmem_size_alltoall pshmem_size_alltoall
#pragma weak shmem_ptrdiff_alltoall = pshmem_ptrdiff_alltoall
#define shmem_ptrdiff_alltoall pshmem_ptrdiff_alltoall
#endif /* ENABLE_PSHMEM */

#define SHMEM_TYPENAME_ALLTOALL(_type, _typename)                              \
  int shmem_##_typename##_alltoall(shmem_team_t team, _type *dest,             \
                                   const _type *source, size_t nelems) {       \
    logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %d)", __func__, team, dest,        \
           source, nelems);                                                    \
    colls.alltoall.f(team, dest, source, nelems);                              \
  }

SHMEM_TYPENAME_ALLTOALL(float, float)
SHMEM_TYPENAME_ALLTOALL(double, double)
SHMEM_TYPENAME_ALLTOALL(long double, longdouble)
SHMEM_TYPENAME_ALLTOALL(char, char)
SHMEM_TYPENAME_ALLTOALL(signed char, schar)
SHMEM_TYPENAME_ALLTOALL(short, short)
SHMEM_TYPENAME_ALLTOALL(int, int)
SHMEM_TYPENAME_ALLTOALL(long, long)
SHMEM_TYPENAME_ALLTOALL(long long, longlong)
SHMEM_TYPENAME_ALLTOALL(unsigned char, uchar)
SHMEM_TYPENAME_ALLTOALL(unsigned short, ushort)
SHMEM_TYPENAME_ALLTOALL(unsigned int, uint)
SHMEM_TYPENAME_ALLTOALL(unsigned long, ulong)
SHMEM_TYPENAME_ALLTOALL(unsigned long long, ulonglong)
SHMEM_TYPENAME_ALLTOALL(int8_t, int8)
SHMEM_TYPENAME_ALLTOALL(int16_t, int16)
SHMEM_TYPENAME_ALLTOALL(int32_t, int32)
SHMEM_TYPENAME_ALLTOALL(int64_t, int64)
SHMEM_TYPENAME_ALLTOALL(uint8_t, uint8)
SHMEM_TYPENAME_ALLTOALL(uint16_t, uint16)
SHMEM_TYPENAME_ALLTOALL(uint32_t, uint32)
SHMEM_TYPENAME_ALLTOALL(uint64_t, uint64)
SHMEM_TYPENAME_ALLTOALL(size_t, size)
SHMEM_TYPENAME_ALLTOALL(ptrdiff_t, ptrdiff)

//////////////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_PSHMEM
#pragma weak shmem_int_alltoalls = pshmem_int_alltoalls
#define shmem_int_alltoalls pshmem_int_alltoalls
#pragma weak shmem_long_alltoalls = pshmem_long_alltoalls
#define shmem_long_alltoalls pshmem_long_alltoalls
#pragma weak shmem_longlong_alltoalls = pshmem_longlong_alltoalls
#define shmem_longlong_alltoalls pshmem_longlong_alltoalls
#pragma weak shmem_float_alltoalls = pshmem_float_alltoalls
#define shmem_float_alltoalls pshmem_float_alltoalls
#pragma weak shmem_double_alltoalls = pshmem_double_alltoalls
#define shmem_double_alltoalls pshmem_double_alltoalls
#pragma weak shmem_longdouble_alltoalls = pshmem_longdouble_alltoalls
#define shmem_longdouble_alltoalls pshmem_longdouble_alltoalls
#pragma weak shmem_uint_alltoalls = pshmem_uint_alltoalls
#define shmem_uint_alltoalls pshmem_uint_alltoalls
#pragma weak shmem_ulong_alltoalls = pshmem_ulong_alltoalls
#define shmem_ulong_alltoalls pshmem_ulong_alltoalls
#pragma weak shmem_ulonglong_alltoalls = pshmem_ulonglong_alltoalls
#define shmem_ulonglong_alltoalls pshmem_ulonglong_alltoalls
#pragma weak shmem_int32_alltoalls = pshmem_int32_alltoalls
#define shmem_int32_alltoalls pshmem_int32_alltoalls
#pragma weak shmem_int64_alltoalls = pshmem_int64_alltoalls
#define shmem_int64_alltoalls pshmem_int64_alltoalls
#pragma weak shmem_uint32_alltoalls = pshmem_uint32_alltoalls
#define shmem_uint32_alltoalls pshmem_uint32_alltoalls
#pragma weak shmem_uint64_alltoalls = pshmem_uint64_alltoalls
#define shmem_uint64_alltoalls pshmem_uint64_alltoalls
#pragma weak shmem_size_alltoalls = pshmem_size_alltoalls
#define shmem_size_alltoalls pshmem_size_alltoalls
#pragma weak shmem_ptrdiff_alltoalls = pshmem_ptrdiff_alltoalls
#define shmem_ptrdiff_alltoalls pshmem_ptrdiff_alltoalls
#endif /* ENABLE_PSHMEM */

#define SHMEM_TYPENAME_ALLTOALLS(_type, _typename)                             \
  int shmem_##_typename##_alltoalls(shmem_team_t team, _type *dest,            \
                                    const _type *source, ptrdiff_t dst,        \
                                    ptrdiff_t sst, size_t nelems) {            \
    logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %p, %p, %d)", __func__, team,      \
           source, dst, sst, nelems);                                          \
    colls.alltoalls.f(team, dest, source, dst, sst, nelems);                   \
  }

SHMEM_TYPENAME_ALLTOALLS(float, float)
SHMEM_TYPENAME_ALLTOALLS(double, double)
SHMEM_TYPENAME_ALLTOALLS(long double, longdouble)
SHMEM_TYPENAME_ALLTOALLS(char, char)
SHMEM_TYPENAME_ALLTOALLS(signed char, schar)
SHMEM_TYPENAME_ALLTOALLS(short, short)
SHMEM_TYPENAME_ALLTOALLS(int, int)
SHMEM_TYPENAME_ALLTOALLS(long, long)
SHMEM_TYPENAME_ALLTOALLS(long long, longlong)
SHMEM_TYPENAME_ALLTOALLS(unsigned char, uchar)
SHMEM_TYPENAME_ALLTOALLS(unsigned short, ushort)
SHMEM_TYPENAME_ALLTOALLS(unsigned int, uint)
SHMEM_TYPENAME_ALLTOALLS(unsigned long, ulong)
SHMEM_TYPENAME_ALLTOALLS(unsigned long long, ulonglong)
SHMEM_TYPENAME_ALLTOALLS(int8_t, int8)
SHMEM_TYPENAME_ALLTOALLS(int16_t, int16)
SHMEM_TYPENAME_ALLTOALLS(int32_t, int32)
SHMEM_TYPENAME_ALLTOALLS(int64_t, int64)
SHMEM_TYPENAME_ALLTOALLS(uint8_t, uint8)
SHMEM_TYPENAME_ALLTOALLS(uint16_t, uint16)
SHMEM_TYPENAME_ALLTOALLS(uint32_t, uint32)
SHMEM_TYPENAME_ALLTOALLS(uint64_t, uint64)
SHMEM_TYPENAME_ALLTOALLS(size_t, size)
SHMEM_TYPENAME_ALLTOALLS(ptrdiff_t, ptrdiff)

//////////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_PSHMEM
#pragma weak shmem_collect32 = pshmem_collect32
#define shmem_collect32 pshmem_collect32
#pragma weak shmem_collect64 = pshmem_collect64
#define shmem_collect64 pshmem_collect64
#endif /* ENABLE_PSHMEM */

void shmem_collect32(void *target, const void *source, size_t nelems,
                     int PE_start, int logPE_stride, int PE_size, long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %lu, %d, %d, %d, %p)", __func__, target,
         source, nelems, PE_start, logPE_stride, PE_size, pSync);

  colls.collect.f32(target, source, nelems, PE_start, logPE_stride, PE_size,
                    pSync);
}

void shmem_collect64(void *target, const void *source, size_t nelems,
                     int PE_start, int logPE_stride, int PE_size, long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %lu, %d, %d, %d, %p)", __func__, target,
         source, nelems, PE_start, logPE_stride, PE_size, pSync);

  colls.collect.f64(target, source, nelems, PE_start, logPE_stride, PE_size,
                    pSync);
}

#ifdef ENABLE_PSHMEM
#pragma weak shmem_fcollect32 = pshmem_fcollect32
#define shmem_fcollect32 pshmem_fcollect32
#pragma weak shmem_fcollect64 = pshmem_fcollect64
#define shmem_fcollect64 pshmem_fcollect64
#endif /* ENABLE_PSHMEM */

void shmem_fcollect32(void *target, const void *source, size_t nelems,
                      int PE_start, int logPE_stride, int PE_size,
                      long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %lu, %d, %d, %d, %p)", __func__, target,
         source, nelems, PE_start, logPE_stride, PE_size, pSync);

  colls.fcollect.f32(target, source, nelems, PE_start, logPE_stride, PE_size,
                     pSync);
}

void shmem_fcollect64(void *target, const void *source, size_t nelems,
                      int PE_start, int logPE_stride, int PE_size,
                      long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %lu, %d, %d, %d, %p)", __func__, target,
         source, nelems, PE_start, logPE_stride, PE_size, pSync);

  colls.fcollect.f64(target, source, nelems, PE_start, logPE_stride, PE_size,
                     pSync);
}

#ifdef ENABLE_PSHMEM
#pragma weak shmem_barrier = pshmem_barrier
#define shmem_barrier pshmem_barrier
#endif /* ENABLE_PSHMEM */

void shmem_barrier(int PE_start, int logPE_stride, int PE_size, long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%d, %d, %d, %p)", __func__, PE_start,
         logPE_stride, PE_size, pSync);

  colls.barrier.f(PE_start, logPE_stride, PE_size, pSync);
}

/*
 * sync variables supplied by me
 */
extern long *shmemc_barrier_all_psync;
extern long *shmemc_sync_all_psync;

#ifdef ENABLE_PSHMEM
#pragma weak shmem_barrier_all = pshmem_barrier_all
#define shmem_barrier_all pshmem_barrier_all
#endif /* ENABLE_PSHMEM */

void shmem_barrier_all(void) {
  logger(LOG_COLLECTIVES, "%s()", __func__);

  colls.barrier_all.f(shmemc_barrier_all_psync);
}

#ifdef ENABLE_PSHMEM
#pragma weak shmem_sync = pshmem_sync
#define shmem_sync pshmem_sync
#endif /* ENABLE_PSHMEM */

void shmem_sync(int PE_start, int logPE_stride, int PE_size, long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%d, %d, %d, %p)", __func__, PE_start,
         logPE_stride, PE_size, pSync);

  colls.sync.f(PE_start, logPE_stride, PE_size, pSync);
}

#ifdef ENABLE_PSHMEM
#pragma weak shmem_sync_all = pshmem_sync_all
#define shmem_sync_all pshmem_sync_all
#endif /* ENABLE_PSHMEM */

void shmem_sync_all(void) {
  logger(LOG_COLLECTIVES, "%s()", __func__);

  colls.sync_all.f(shmemc_sync_all_psync);
}

#ifdef ENABLE_PSHMEM
#pragma weak shmem_broadcast32 = pshmem_broadcast32
#define shmem_broadcast32 pshmem_broadcast32
#pragma weak shmem_broadcast64 = pshmem_broadcast64
#define shmem_broadcast64 pshmem_broadcast64
#endif /* ENABLE_PSHMEM */

void shmem_broadcast32(void *target, const void *source, size_t nelems,
                       int PE_root, int PE_start, int logPE_stride, int PE_size,
                       long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %lu, %d, %d, %d, %p)", __func__, target,
         source, nelems, PE_start, logPE_stride, PE_size, pSync);

  colls.broadcast.f32(target, source, nelems, PE_root, PE_start, logPE_stride,
                      PE_size, pSync);
}

void shmem_broadcast64(void *target, const void *source, size_t nelems,
                       int PE_root, int PE_start, int logPE_stride, int PE_size,
                       long *pSync) {
  logger(LOG_COLLECTIVES, "%s(%p, %p, %lu, %d, %d, %d, %p)", __func__, target,
         source, nelems, PE_start, logPE_stride, PE_size, pSync);

  colls.broadcast.f64(target, source, nelems, PE_root, PE_start, logPE_stride,
                      PE_size, pSync);
}

/*
 * -- WIP ----------------------------------------------------------
 */

#include "collectives/reductions.h"

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
SHIM_REDUCE_ALL(rec_dbl)
