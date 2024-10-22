//
// Created by Srdan Milakovic on 5/21/18.
//

#include "shcoll.h"
#include "shcoll/compat.h"

#include "shmem.h"

#include <limits.h>
#include <assert.h>

inline static int edge_color(int i, int me, int npes) {
  int chr_idx;
  int v;

  chr_idx = npes % 2 == 1 ? npes : npes - 1;
  if (me < chr_idx) {
    v = (i + chr_idx - me) % chr_idx;
  } else {
    v = i % 2 == 1 ? (((i + chr_idx) / 2) % chr_idx) : i / 2;
  }

  if (npes % 2 == 1 && v == me) {
    return -1;
  } else if (v == me) {
    return chr_idx;
  } else {
    return v;
  }
}

// TODO: needs to return -1 if unsuccessful
#define ALLTOALLS_HELPER_BARRIER_DEFINITION(_algo, _peer, _cond)                \
  inline static int alltoalls_helper_##_algo##_barrier(                         \
      void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,             \
      size_t nelems, int PE_start, int logPE_stride, int PE_size) {             \
    const int stride = 1 << logPE_stride;                                       \
    const int me = shmem_my_pe();                                               \
                                                                                \
    /* Get my index in the active set */                                        \
    const int me_as = (me - PE_start) / stride;                                 \
                                                                                \
    void *const dest_ptr = ((uint8_t *)dest) + me_as * dst * nelems;            \
    void const *source_ptr = ((uint8_t *)source) + me_as * sst * nelems;        \
                                                                                \
    for (int i = 0; i < nelems; i++) {                                          \
      *(((uint8_t *)dest_ptr) + i * dst) =                                      \
          *(((uint8_t *)source_ptr) + i * sst);                                 \
    }                                                                           \
                                                                                \
    for (int i = 1; i < PE_size; i++) {                                         \
      int peer_as = _peer(i, me_as, PE_size);                                   \
      source_ptr = ((uint8_t *)source) + peer_as * sst * nelems;                \
                                                                                \
      shmem_iput(dest_ptr, source_ptr, dst, sst, nelems,                        \
                 PE_start + peer_as * stride);                                  \
    }                                                                           \
                                                                                \
    shmem_team_sync(SHMEM_TEAM_WORLD);                                          \
    return 0;                                                                   \
  }

// TODO: needs to return -1 if unsuccessful
#define ALLTOALLS_HELPER_COUNTER_DEFINITION(_algo, _peer, _cond)                \
  inline static int alltoalls_helper_##_algo##_counter(                         \
      void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,             \
      size_t nelems, int PE_start, int logPE_stride, int PE_size) {             \
    const int stride = 1 << logPE_stride;                                       \
    const int me = shmem_my_pe();                                               \
                                                                                \
    /* Get my index in the active set */                                        \
    const int me_as = (me - PE_start) / stride;                                 \
                                                                                \
    void *const dest_ptr = ((uint8_t *)dest) + me_as * dst * nelems;            \
    void const *source_ptr = ((uint8_t *)source) + me_as * sst * nelems;        \
                                                                                \
    for (int i = 0; i < nelems; i++) {                                          \
      *(((uint8_t *)dest_ptr) + i * dst) =                                      \
          *(((uint8_t *)source_ptr) + i * sst);                                 \
    }                                                                           \
                                                                                \
    for (int i = 1; i < PE_size; i++) {                                         \
      int peer_as = _peer(i, me_as, PE_size);                                   \
      source_ptr = ((uint8_t *)source) + peer_as * sst * nelems;                \
                                                                                \
      shmem_iput(dest_ptr, source_ptr, dst, sst, nelems,                        \
                 PE_start + peer_as * stride);                                  \
    }                                                                           \
                                                                                \
    shmem_fence();                                                              \
                                                                                \
    for (int i = 1; i < PE_size; i++) {                                         \
      int peer_as = _peer(i, me_as, PE_size);                                   \
      shmem_long_atomic_inc(pSync, PE_start + peer_as * stride);                \
    }                                                                           \
                                                                                \
    shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE + PE_size - 1);\
    shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);                                 \
    return 0;                                                                   \
  }

// TODO: needs to return -1 if unsuccessful
#define ALLTOALLS_HELPER_SIGNAL_DEFINITION(_algo, _peer, _cond)                 \
  inline static int alltoalls_helper_##_algo##_signal(                          \
      void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,             \
      size_t nelems, int PE_start, int logPE_stride, int PE_size) {             \
    const int stride = 1 << logPE_stride;                                       \
    const int me = shmem_my_pe();                                               \
                                                                                \
    /* Get my index in the active set */                                        \
    const int me_as = (me - PE_start) / stride;                                 \
                                                                                \
    void *const dest_ptr = ((uint8_t *)dest) + me_as * dst * nelems;            \
    void const *source_ptr = ((uint8_t *)source) + me_as * sst * nelems;        \
                                                                                \
    for (int i = 0; i < nelems; i++) {                                          \
      *(((uint8_t *)dest_ptr) + i * dst) =                                      \
          *(((uint8_t *)source_ptr) + i * sst);                                 \
    }                                                                           \
                                                                                \
    for (int i = 1; i < PE_size; i++) {                                         \
      int peer_as = _peer(i, me_as, PE_size);                                   \
      source_ptr = ((uint8_t *)source) + peer_as * sst * nelems;                \
                                                                                \
      shmem_iput(dest_ptr, source_ptr, dst, sst, nelems,                        \
                 PE_start + peer_as * stride);                                  \
    }                                                                           \
                                                                                \
    shmem_team_sync(SHMEM_TEAM_WORLD);                                          \
    return 0;                                                                   \
  }


// TODO:: Call helper definitions here
#define SHIFT_PEER(I, ME, NPES) (((ME) + (I)) % (NPES))
ALLTOALLS_HELPER_BARRIER_DEFINITION(shift_exchange, SHIFT_PEER, 1)
ALLTOALLS_HELPER_COUNTER_DEFINITION(shift_exchange, SHIFT_PEER, 1)
ALLTOALLS_HELPER_SIGNAL_DEFINITION(shift_exchange, SHIFT_PEER,
                                  PE_size - 1 <= SHCOLL_ALLTOALLS_SYNC_SIZE)

#define XOR_PEER(I, ME, NPES) ((I) ^ (ME))
#define XOR_COND (((PE_size - 1) & PE_size) == 0)

ALLTOALLS_HELPER_BARRIER_DEFINITION(xor_pairwise_exchange, XOR_PEER, XOR_COND)
ALLTOALLS_HELPER_COUNTER_DEFINITION(xor_pairwise_exchange, XOR_PEER, XOR_COND)
ALLTOALLS_HELPER_SIGNAL_DEFINITION(xor_pairwise_exchange, XOR_PEER,
                                  XOR_COND &&PE_size - 1 <=
                                      SHCOLL_ALLTOALLS_SYNC_SIZE)

#define COLOR_PEER(I, ME, NPES) edge_color(I, ME, NPES)
#define COLOR_COND (PE_size % 2 == 0)

ALLTOALLS_HELPER_BARRIER_DEFINITION(color_pairwise_exchange, COLOR_PEER,
                                   COLOR_COND)
ALLTOALLS_HELPER_COUNTER_DEFINITION(color_pairwise_exchange, COLOR_PEER,
                                   COLOR_COND)
ALLTOALLS_HELPER_SIGNAL_DEFINITION(color_pairwise_exchange, COLOR_PEER,
                                  (PE_size - 1 <= SHCOLL_ALLTOALLS_SYNC_SIZE) &&
                                      COLOR_COND)


#define SHCOLL_ALLTOALLS_TYPED_DEFINITION(_algo, _type, _typename)               \
  int shcoll_##_typename##_alltoalls_##_algo(                                    \
      shmem_team_t team, _type *dest, const _type *source, ptrdiff_t dst,        \
      ptrdiff_t sst, size_t nelems) {                                            \
    int PE_start = shmem_team_translate_pe(team, 0, SHMEM_TEAM_WORLD);           \
    int logPE_stride = 0;                                                        \
    int PE_size = shmem_team_n_pes(team);                                        \
    int ret = alltoalls_helper_##_algo(dest, source, dst, sst,                   \
                                       sizeof(_type) * nelems,                   \
                                       PE_start, logPE_stride, PE_size);         \
    if (ret != 0) {                                                              \
      return -1;                                                                 \
    }                                                                            \
    return 0;                                                                    \
  }

SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, float, float)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, double, double)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, long double, longdouble)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, char, char)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, signed char, schar)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, short, short)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, int, int)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, long, long)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, long long, longlong)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, unsigned char, uchar)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, unsigned short, ushort)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, unsigned int, uint)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, unsigned long, ulong)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, unsigned long long, ulonglong)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, int8_t, int8)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, int16_t, int16)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, int32_t, int32)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, int64_t, int64)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, uint8_t, uint8)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, uint16_t, uint16)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, uint32_t, uint32)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, uint64_t, uint64)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, size_t, size)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(shift_exchange_barrier, ptrdiff_t, ptrdiff)

SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, float, float)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, double, double)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, long double, longdouble)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, char, char)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, signed char, schar)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, short, short)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, int, int)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, long, long)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, long long, longlong)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, unsigned char, uchar)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, unsigned short, ushort)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, unsigned int, uint)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, unsigned long, ulong)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, unsigned long long, ulonglong)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, int8_t, int8)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, int16_t, int16)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, int32_t, int32)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, int64_t, int64)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, uint8_t, uint8)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, uint16_t, uint16)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, uint32_t, uint32)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, uint64_t, uint64)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, size_t, size)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(xor_pairwise_exchange_barrier, ptrdiff_t, ptrdiff)

SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, float, float)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, double, double)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, long double, longdouble)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, char, char)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, signed char, schar)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, short, short)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, int, int)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, long, long)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, long long, longlong)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, unsigned char, uchar)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, unsigned short, ushort)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, unsigned int, uint)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, unsigned long, ulong)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, unsigned long long, ulonglong)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, int8_t, int8)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, int16_t, int16)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, int32_t, int32)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, int64_t, int64)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, uint8_t, uint8)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, uint16_t, uint16)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, uint32_t, uint32)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, uint64_t, uint64)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, size_t, size)
SHCOLL_ALLTOALLS_TYPED_DEFINITION(color_pairwise_exchange_barrier, ptrdiff_t, ptrdiff)
































// #define SHCOLL_ALLTOALLS_BARRIER_DEFINITION(_name, _size, _peer, _cond, _nbi)  \
//   void shcoll_alltoalls##_size##_##_name##_barrier##_nbi(                      \
//       void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,            \
//       size_t nelems, int PE_start, int logPE_stride, int PE_size,              \
//       long *pSync) {                                                           \
//     const int stride = 1 << logPE_stride;                                      \
//     const int me = shmem_my_pe();                                              \
//                                                                                \
//     /* Get my index in the active set */                                       \
//     /* const int me_as = (me - PE_start) / stride; */                          \
//                                                                                \
//     void *const dest_ptr =                                                     \
//         ((uint8_t *)dest) + me * dst * nelems * ((_size) / CHAR_BIT);          \
//     void const *source_ptr =                                                   \
//         ((uint8_t *)source) + me * sst * nelems * ((_size) / CHAR_BIT);        \
//                                                                                \
//     int i;                                                                     \
//     int peer_as;                                                               \
//                                                                                \
//     assert(_cond);                                                             \
//                                                                                \
//     for (i = 0; i < nelems; i++) {                                             \
//       *(((uint##_size##_t *)dest_ptr) + i * dst) =                             \
//           *(((uint##_size##_t *)source_ptr) + i * sst);                        \
//     }                                                                          \
//                                                                                \
//     for (i = 1; i < PE_size; i++) {                                            \
//       peer_as = _peer(i, me, PE_size);                                         \
//       source_ptr =                                                             \
//           ((uint8_t *)source) + peer_as * sst * nelems * ((_size) / CHAR_BIT); \
//                                                                                \
//       shmem_iput##_size##_nbi(dest_ptr, source_ptr, dst, sst, nelems,          \
//                               PE_start + peer_as * stride);                    \
//     }                                                                          \
//                                                                                \
//     /* TODO: change to auto shcoll barrier */                                  \
//     shcoll_barrier_binomial_tree(PE_start, logPE_stride, PE_size, pSync);      \
//   }

// #define SHCOLL_ALLTOALLS_COUNTER_DEFINITION(_name, _size, _peer, _cond, _nbi)  \
//   void shcoll_alltoalls##_size##_##_name##_counter##_nbi(                      \
//       void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,            \
//       size_t nelems, int PE_start, int logPE_stride, int PE_size,              \
//       long *pSync) {                                                           \
//     const int stride = 1 << logPE_stride;                                      \
//     const int me = shmem_my_pe();                                              \
//                                                                                \
//     /* Get my index in the active set */                                       \
//     const int me_as = (me - PE_start) / stride;                                \
//                                                                                \
//     void *const dest_ptr =                                                     \
//         ((uint8_t *)dest) + me * dst * nelems * ((_size) / CHAR_BIT);          \
//     void const *source_ptr =                                                   \
//         ((uint8_t *)source) + me * sst * nelems * ((_size) / CHAR_BIT);        \
//                                                                                \
//     int i;                                                                     \
//     int peer_as;                                                               \
//                                                                                \
//     assert(_cond);                                                             \
//                                                                                \
//     for (i = 0; i < nelems; i++) {                                             \
//       *(((uint##_size##_t *)dest_ptr) + i * dst) =                             \
//           *(((uint##_size##_t *)source_ptr) + i * sst);                        \
//     }                                                                          \
//                                                                                \
//     for (i = 1; i < PE_size; i++) {                                            \
//       peer_as = _peer(i, me, PE_size);                                         \
//       source_ptr =                                                             \
//           ((uint8_t *)source) + peer_as * sst * nelems * ((_size) / CHAR_BIT); \
//                                                                                \
//       shmem_iput##_size##_nbi(dest_ptr, source_ptr, dst, sst, nelems,          \
//                               PE_start + peer_as * stride);                    \
//     }                                                                          \
//                                                                                \
//     shmem_fence();                                                             \
//                                                                                \
//     for (i = 1; i < PE_size; i++) {                                            \
//       peer_as = _peer(i, me_as, PE_size);                                      \
//       shmem_long_atomic_inc(pSync, PE_start + peer_as * stride);               \
//     }                                                                          \
//                                                                                \
//     shmem_long_wait_until(pSync, SHMEM_CMP_EQ,                                 \
//                           SHCOLL_SYNC_VALUE + PE_size - 1);                    \
//     shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);                                \
//   }

// #define SHCOLL_ALLTOALLS_DEFINITION(_macro, _name, _peer, _cond, _nbi)          \
//   _macro(_name, 32, _peer, _cond, _nbi) _macro(_name, 64, _peer, _cond, _nbi)

// // @formatter:off

// #define SHIFT_PEER(I, ME, NPES) (((ME) + (I)) % (NPES))

// SHCOLL_ALLTOALLS_DEFINITION(SHCOLL_ALLTOALLS_BARRIER_DEFINITION,
// shift_exchange,
//                            SHIFT_PEER, 1, )
// SHCOLL_ALLTOALLS_DEFINITION(SHCOLL_ALLTOALLS_BARRIER_DEFINITION,
// shift_exchange,
//                            SHIFT_PEER, 1, _nbi)

// SHCOLL_ALLTOALLS_DEFINITION(SHCOLL_ALLTOALLS_COUNTER_DEFINITION,
// shift_exchange,
//                            SHIFT_PEER, 1, )
// SHCOLL_ALLTOALLS_DEFINITION(SHCOLL_ALLTOALLS_COUNTER_DEFINITION,
// shift_exchange,
//                            SHIFT_PEER, 1, _nbi)

// #define XOR_PEER(I, ME, NPES) ((I) ^ (ME))
// #define XOR_COND (((PE_size - 1) & PE_size) == 0)

// SHCOLL_ALLTOALLS_DEFINITION(SHCOLL_ALLTOALLS_BARRIER_DEFINITION,
//                            xor_pairwise_exchange, SHIFT_PEER, XOR_COND, )
// SHCOLL_ALLTOALLS_DEFINITION(SHCOLL_ALLTOALLS_BARRIER_DEFINITION,
//                            xor_pairwise_exchange, SHIFT_PEER, XOR_COND, _nbi)

// SHCOLL_ALLTOALLS_DEFINITION(SHCOLL_ALLTOALLS_COUNTER_DEFINITION,
//                            xor_pairwise_exchange, SHIFT_PEER, XOR_COND, )
// SHCOLL_ALLTOALLS_DEFINITION(SHCOLL_ALLTOALLS_COUNTER_DEFINITION,
//                            xor_pairwise_exchange, SHIFT_PEER, XOR_COND, _nbi)

// #define COLOR_PEER(I, ME, NPES) edge_color(I, ME, NPES)

// SHCOLL_ALLTOALLS_DEFINITION(SHCOLL_ALLTOALLS_BARRIER_DEFINITION,
//                            color_pairwise_exchange, SHIFT_PEER, 1, )
// SHCOLL_ALLTOALLS_DEFINITION(SHCOLL_ALLTOALLS_BARRIER_DEFINITION,
//                            color_pairwise_exchange, SHIFT_PEER, 1, _nbi)

// SHCOLL_ALLTOALLS_DEFINITION(SHCOLL_ALLTOALLS_COUNTER_DEFINITION,
//                            color_pairwise_exchange, SHIFT_PEER, 1, )
// SHCOLL_ALLTOALLS_DEFINITION(SHCOLL_ALLTOALLS_COUNTER_DEFINITION,
//                            color_pairwise_exchange, SHIFT_PEER, 1, _nbi)

// // @formatter:on
