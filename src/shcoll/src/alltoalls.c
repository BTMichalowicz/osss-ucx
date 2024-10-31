/* For license: see LICENSE file at top-level */

#include "shcoll.h"
#include "shcoll/compat.h"

#include <string.h>
#include <limits.h>
#include <assert.h>

#define ALLTOALLS_HELPER_BARRIER_DEFINITION(_algo, _peer, _cond)               \
  inline static int alltoalls_helper_##_algo##_barrier(                        \
      void *dest, const void *source, ptrdiff_t dst_stride,                    \
      ptrdiff_t sst_stride, size_t nelems, int PE_start, int logPE_stride,     \
      int PE_size) {                                                           \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
    uint8_t *dest_ptr = (uint8_t *)dest;                                       \
    const uint8_t *source_ptr = (const uint8_t *)source;                       \
                                                                               \
    /* Get my index in the active set */                                       \
    int me_as = (me - PE_start) / stride;                                      \
    if (me_as < 0 || me_as >= PE_size || _cond == 0) {                         \
      return -1;                                                               \
    }                                                                          \
                                                                               \
    /* Send my data to each PE */                                              \
    for (int i = 0; i < PE_size; i++) {                                        \
      int peer = PE_start + i * stride;                                        \
      const uint8_t *src = source_ptr + i * sst_stride;                        \
      uint8_t *dst = dest_ptr + me_as * dst_stride;                            \
      shmem_putmem(dst, src, nelems, peer);                                    \
    }                                                                          \
                                                                               \
    shmem_fence();                                                             \
    shmem_barrier_all();                                                       \
                                                                               \
    return 0;                                                                  \
  }

// FIXME: test to make sure this works
#define ALLTOALLS_HELPER_SIGNAL_DEFINITION(_algo, _peer, _cond)                \
  inline static int alltoalls_helper_##_algo##_signal(                         \
      void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,            \
      size_t nelems, int PE_start, int logPE_stride, int PE_size) {            \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
    uint8_t *dest_ptr = (uint8_t *)dest;                                       \
    const uint8_t *source_ptr = (uint8_t *)source;                             \
    uint64_t *signal_ptr;                                                      \
                                                                               \
    /* Get my index in the active set */                                       \
    int me_as = (me - PE_start) / stride;                                      \
    if (me_as < 0 || me_as >= PE_size || _cond == 0) {                         \
      return -1;                                                               \
    }                                                                          \
                                                                               \
    /* Copy local data with stride */                                          \
    for (size_t i = 0; i < nelems; i++) {                                      \
      memcpy(dest_ptr + (me_as * dst + i) * nelems,                            \
             source_ptr + (me_as * sst + i) * nelems, nelems);                 \
    }                                                                          \
                                                                               \
    /* Exchange data with other PEs using signal */                            \
    for (int i = 0; i < PE_size; ++i) {                                        \
      if (i == me_as)                                                          \
        continue;                                                              \
                                                                               \
      const int peer_as = _peer(i, me_as, PE_size);                            \
      const int peer = PE_start + peer_as * stride;                            \
                                                                               \
      /* Ensure signal address is properly aligned */                          \
      signal_ptr = (uint64_t *)(dest_ptr + (me_as * dst + 1) * nelems);        \
                                                                               \
      /* Put data to peer with stride and signal */                            \
      shmem_putmem_signal_nbi(dest_ptr + me_as * dst * nelems,                 \
                              source_ptr + peer_as * sst * nelems, nelems,     \
                              signal_ptr, SHCOLL_SYNC_VALUE + 1,               \
                              SHMEM_SIGNAL_SET, peer);                         \
    }                                                                          \
                                                                               \
    /* Wait for all signals */                                                 \
    shmem_team_sync(SHMEM_TEAM_WORLD);                                         \
    return 0;                                                                  \
  }

// FIXME: test to make sure this works
#define ALLTOALLS_HELPER_COUNTER_DEFINITION(_algo, _peer, _cond)               \
  inline static int alltoalls_helper_##_algo##_counter(                        \
      void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,            \
      size_t nelems, int PE_start, int logPE_stride, int PE_size) {            \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
    uint8_t *dest_ptr = (uint8_t *)dest;                                       \
    const uint8_t *source_ptr = (uint8_t *)source;                             \
                                                                               \
    /* Get my index in the active set */                                       \
    int me_as = (me - PE_start) / stride;                                      \
    if (me_as < 0 || me_as >= PE_size || _cond == 0) {                         \
      return -1;                                                               \
    }                                                                          \
                                                                               \
    /* Copy local data with stride */                                          \
    for (size_t i = 0; i < nelems; i++) {                                      \
      memcpy(dest_ptr + (me_as * dst + i) * nelems,                            \
             source_ptr + (me_as * sst + i) * nelems, nelems);                 \
    }                                                                          \
                                                                               \
    /* Exchange data with counter-based synchronization */                     \
    static long counter = 0;                                                   \
    for (int i = 0; i < PE_size; ++i) {                                        \
      if (i == me_as)                                                          \
        continue;                                                              \
                                                                               \
      const int peer_as = _peer(i, me_as, PE_size);                            \
      const int peer = PE_start + peer_as * stride;                            \
                                                                               \
      shmem_putmem_nbi(dest_ptr + me_as * dst * nelems,                        \
                       source_ptr + peer_as * sst * nelems, nelems, peer);     \
      shmem_fence();                                                           \
      shmem_long_atomic_inc(&counter, peer);                                   \
    }                                                                          \
                                                                               \
    /* Wait for all counters */                                                \
    shmem_long_wait_until(&counter, SHMEM_CMP_EQ, PE_size - 1);                \
    counter = 0;                                                               \
    return 0;                                                                  \
  }

#define SHIFT_PEER(I, ME, NPES) (((ME) + (I)) % (NPES))
#define XOR_PEER(I, ME, NPES) ((ME) ^ (I))
#define COLOR_PEER(I, ME, NPES) ((((ME) / 2) * 2 + 1 - ((ME) % 2)) ^ (I))
#define XOR_COND 1
#define COLOR_COND ((me_as % 2) == 0 || (me_as + 1) < PE_size)

// clang-format off
ALLTOALLS_HELPER_BARRIER_DEFINITION(shift_exchange, SHIFT_PEER, 1)
ALLTOALLS_HELPER_SIGNAL_DEFINITION(shift_exchange, SHIFT_PEER, 1)
ALLTOALLS_HELPER_COUNTER_DEFINITION(shift_exchange, SHIFT_PEER, 1)

ALLTOALLS_HELPER_BARRIER_DEFINITION(xor_pairwise_exchange, XOR_PEER, XOR_COND)
ALLTOALLS_HELPER_SIGNAL_DEFINITION(xor_pairwise_exchange, XOR_PEER, XOR_COND)
ALLTOALLS_HELPER_COUNTER_DEFINITION(xor_pairwise_exchange, XOR_PEER, XOR_COND)

ALLTOALLS_HELPER_BARRIER_DEFINITION(color_pairwise_exchange, COLOR_PEER, COLOR_COND)
ALLTOALLS_HELPER_SIGNAL_DEFINITION(color_pairwise_exchange, COLOR_PEER, COLOR_COND)
ALLTOALLS_HELPER_COUNTER_DEFINITION(color_pairwise_exchange, COLOR_PEER, COLOR_COND)
// clang-format on

#define SHCOLL_ALLTOALLS_DEFINITION(_algo, _type, _typename)                   \
  int shcoll_##_typename##_alltoalls_##_algo(                                  \
      shmem_team_t team, _type *dest, const _type *source, ptrdiff_t dst,      \
      ptrdiff_t sst, size_t nelems) {                                          \
    int PE_start = shmem_team_translate_pe(team, 0, SHMEM_TEAM_WORLD);         \
    int logPE_stride = 0;                                                      \
    int PE_size = shmem_team_n_pes(team);                                      \
    int ret = alltoalls_helper_##_algo(                                        \
        dest, source, dst * sizeof(_type), sst * sizeof(_type),                \
        nelems * sizeof(_type), PE_start, logPE_stride, PE_size);              \
    if (ret != 0) {                                                            \
      return -1;                                                               \
    }                                                                          \
    return 0;                                                                  \
  }

// #define SHCOLL_ALLTOALLS_DEFINITION(_algo, _type, _typename)                   \
//   int shcoll_##_typename##_alltoalls_##_algo(                                  \
//       shmem_team_t team, _type *dest, const _type *source,                     \
//       ptrdiff_t dst, ptrdiff_t sst, size_t nelems) {                          \
//     int PE_start = shmem_team_translate_pe(team, 0, SHMEM_TEAM_WORLD);        \
//     int logPE_stride = 0;                                                      \
//     int PE_size = shmem_team_n_pes(team);                                     \
//     return alltoalls_helper_##_algo(dest, source,                             \
//                                    dst * sizeof(_type),                        \
//                                    sst * sizeof(_type),                        \
//                                    nelems * sizeof(_type),                     \
//                                    PE_start, logPE_stride, PE_size);           \
//   }

// clang-format off

// TODO: make this less redundant

/* shift_exchange_barrier */
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, float, float)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, double, double)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, long double, longdouble)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, char, char)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, signed char, schar)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, short, short)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, int, int)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, long, long)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, long long, longlong)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, unsigned char, uchar)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, unsigned short, ushort)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, unsigned int, uint)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, unsigned long, ulong)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, unsigned long long, ulonglong)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, int8_t, int8)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, int16_t, int16)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, int32_t, int32)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, int64_t, int64)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, uint8_t, uint8)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, uint16_t, uint16)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, uint32_t, uint32)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, uint64_t, uint64)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, size_t, size)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_barrier, ptrdiff_t, ptrdiff)

/* shift_exchange_counter */
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, float, float)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, double, double)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, long double, longdouble)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, char, char)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, signed char, schar)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, short, short)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, int, int)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, long, long)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, long long, longlong)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, unsigned char, uchar)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, unsigned short, ushort)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, unsigned int, uint)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, unsigned long, ulong)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, unsigned long long, ulonglong)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, int8_t, int8)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, int16_t, int16)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, int32_t, int32)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, int64_t, int64)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, uint8_t, uint8)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, uint16_t, uint16)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, uint32_t, uint32)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, uint64_t, uint64)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, size_t, size)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_counter, ptrdiff_t, ptrdiff)

/* shift_exchange_signal */
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, float, float)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, double, double)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, long double, longdouble)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, char, char)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, signed char, schar)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, short, short)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, int, int)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, long, long)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, long long, longlong)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, unsigned char, uchar)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, unsigned short, ushort)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, unsigned int, uint)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, unsigned long, ulong)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, unsigned long long, ulonglong)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, int8_t, int8)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, int16_t, int16)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, int32_t, int32)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, int64_t, int64)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, uint8_t, uint8)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, uint16_t, uint16)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, uint32_t, uint32)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, uint64_t, uint64)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, size_t, size)
SHCOLL_ALLTOALLS_DEFINITION(shift_exchange_signal, ptrdiff_t, ptrdiff)

/* xor_pairwise_exchange_barrier */
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, float, float)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, double, double)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, long double, longdouble)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, char, char)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, signed char, schar)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, short, short)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, int, int)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, long, long)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, long long, longlong)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, unsigned char, uchar)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, unsigned short, ushort)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, unsigned int, uint)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, unsigned long, ulong)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, unsigned long long, ulonglong)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, int8_t, int8)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, int16_t, int16)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, int32_t, int32)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, int64_t, int64)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, uint8_t, uint8)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, uint16_t, uint16)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, uint32_t, uint32)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, uint64_t, uint64)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, size_t, size)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_barrier, ptrdiff_t, ptrdiff)

/* xor_pairwise_exchange_counter */
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, float, float)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, double, double)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, long double, longdouble)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, char, char)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, signed char, schar)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, short, short)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, int, int)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, long, long)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, long long, longlong)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, unsigned char, uchar)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, unsigned short, ushort)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, unsigned int, uint)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, unsigned long, ulong)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, unsigned long long, ulonglong)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, int8_t, int8)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, int16_t, int16)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, int32_t, int32)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, int64_t, int64)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, uint8_t, uint8)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, uint16_t, uint16)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, uint32_t, uint32)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, uint64_t, uint64)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, size_t, size)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_counter, ptrdiff_t, ptrdiff)

/* xor_pairwise_exchange_signal */
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, float, float)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, double, double)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, long double, longdouble)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, char, char)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, signed char, schar)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, short, short)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, int, int)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, long, long)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, long long, longlong)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, unsigned char, uchar)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, unsigned short, ushort)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, unsigned int, uint)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, unsigned long, ulong)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, unsigned long long, ulonglong)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, int8_t, int8)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, int16_t, int16)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, int32_t, int32)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, int64_t, int64)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, uint8_t, uint8)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, uint16_t, uint16)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, uint32_t, uint32)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, uint64_t, uint64)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, size_t, size)
SHCOLL_ALLTOALLS_DEFINITION(xor_pairwise_exchange_signal, ptrdiff_t, ptrdiff)

/* color_pairwise_exchange_barrier */
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, float, float)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, double, double)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, long double, longdouble)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, char, char)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, signed char, schar)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, short, short)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, int, int)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, long, long)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, long long, longlong)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, unsigned char, uchar)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, unsigned short, ushort)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, unsigned int, uint)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, unsigned long, ulong)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, unsigned long long, ulonglong)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, int8_t, int8)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, int16_t, int16)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, int32_t, int32)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, int64_t, int64)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, uint8_t, uint8)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, uint16_t, uint16)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, uint32_t, uint32)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, uint64_t, uint64)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, size_t, size)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_barrier, ptrdiff_t, ptrdiff)

/* color_pairwise_exchange_counter */
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, float, float)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, double, double)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, long double, longdouble)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, char, char)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, signed char, schar)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, short, short)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, int, int)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, long, long)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, long long, longlong)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, unsigned char, uchar)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, unsigned short, ushort)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, unsigned int, uint)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, unsigned long, ulong)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, unsigned long long, ulonglong)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, int8_t, int8)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, int16_t, int16)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, int32_t, int32)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, int64_t, int64)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, uint8_t, uint8)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, uint16_t, uint16)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, uint32_t, uint32)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, uint64_t, uint64)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, size_t, size)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_counter, ptrdiff_t, ptrdiff)

/* color_pairwise_exchange_signal */
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, float, float)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, double, double)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, long double, longdouble)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, char, char)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, signed char, schar)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, short, short)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, int, int)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, long, long)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, long long, longlong)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, unsigned char, uchar)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, unsigned short, ushort)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, unsigned int, uint)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, unsigned long, ulong)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, unsigned long long, ulonglong)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, int8_t, int8)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, int16_t, int16)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, int32_t, int32)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, int64_t, int64)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, uint8_t, uint8)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, uint16_t, uint16)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, uint32_t, uint32)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, uint64_t, uint64)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, size_t, size)
SHCOLL_ALLTOALLS_DEFINITION(color_pairwise_exchange_signal, ptrdiff_t, ptrdiff)

// clang-format on
