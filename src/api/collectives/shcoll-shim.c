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
#include "shmem/teams.h"
#include "shmem/api_types.h"

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
 * @brief Helper macro to call a typed collective operation
 * @param CONFIG The collective operation name
 * @param TYPENAME The type name
 * @param ... The arguments to the collective operation
 */
#define TYPED_CALL(CONFIG, TYPENAME, ...)                                      \
  do {                                                                         \
    char opstr[COLL_NAME_MAX * 2];                                             \
    const char *base = proc.env.coll.CONFIG;                                   \
    if (strchr(base, ':') == NULL) {                                           \
      snprintf(opstr, sizeof(opstr), "%s:%s", base, TYPENAME);                 \
    } else {                                                                   \
      strncpy(opstr, base, sizeof(opstr) - 1);                                 \
      opstr[sizeof(opstr) - 1] = '\0';                                         \
    }                                                                          \
    int _rc = register_##CONFIG(opstr);                                        \
    if (_rc != 0) {                                                            \
      shmemu_fatal("couldn't register typed collective '%s' (s = %d)", opstr,  \
                   _rc);                                                       \
    }                                                                          \
    return colls.CONFIG.f(__VA_ARGS__);                                        \
  } while (0)

/**
 * @brief Macro for to_all typed call operations with void return type
 * @param CONFIG The collective configuration
 * @param TYPENAME The type name string
 * @param ... Additional arguments to pass to the operation
 */
#define TO_ALL_TYPED_CALL(CONFIG, TYPENAME, ...)                               \
  do {                                                                         \
    char opstr[COLL_NAME_MAX * 2];                                             \
    const char *base = proc.env.coll.CONFIG;                                   \
    if (strchr(base, ':') == NULL) {                                           \
      snprintf(opstr, sizeof(opstr), "%s:%s", base, TYPENAME);                 \
    } else {                                                                   \
      strncpy(opstr, base, sizeof(opstr) - 1);                                 \
      opstr[sizeof(opstr) - 1] = '\0';                                         \
    }                                                                          \
    int _rc = register_##CONFIG(opstr);                                        \
    if (_rc != 0) {                                                            \
      shmemu_fatal("couldn't register typed collective '%s' (s = %d)", opstr,  \
                   _rc);                                                       \
    }                                                                          \
    colls.CONFIG.f(__VA_ARGS__);                                               \
    return;                                                                    \
  } while (0)

/**
 * @brief Initialize all collective operations
 *
 * Registers implementations for all collective operations including:
 * alltoall, alltoalls, collect, fcollect, barrier, sync, and broadcast
 */
void collectives_init(void) {
  TRY(alltoall_type);
  TRY(alltoall_mem);
  TRY(alltoall_size);

  TRY(alltoalls_type);
  TRY(alltoalls_mem);
  TRY(alltoalls_size);

  TRY(collect_type);
  TRY(collect_mem);
  TRY(collect_size);

  TRY(fcollect_type);
  TRY(fcollect_mem);
  TRY(fcollect_size);

  TRY(broadcast_type);
  TRY(broadcast_mem);
  TRY(broadcast_size);

  TRY(barrier);
  TRY(barrier_all);
  TRY(sync);
  TRY(team_sync);
  TRY(sync_all);

  TRY(and_to_all);
  TRY(or_to_all);
  TRY(xor_to_all);
  TRY(max_to_all);
  TRY(min_to_all);
  TRY(sum_to_all);
  TRY(prod_to_all);

  TRY(and_reduce);
  TRY(or_reduce);
  TRY(xor_reduce);
  TRY(max_reduce);
  TRY(min_reduce);
  TRY(sum_reduce);
  TRY(prod_reduce);
}

/**
 * @brief Cleanup and finalize collective operations
 */
void collectives_finalize(void) { return; }

/**
 * @defgroup alltoall All-to-all Operations
 * @{
 */

#ifdef ENABLE_PSHMEM
#define DECL_PSHMEM_WEAK_ALLTOALL(_type, _typename)                            \
  _Pragma("weak shmem_" #_typename "_alltoall = pshmem_" #_typename            \
          "_alltoall") #define shmem_##_typename##_alltoall                    \
      pshmem_##_typename##_alltoall
SHMEM_STANDARD_RMA_TYPE_TABLE(DECL_PSHMEM_WEAK_ALLTOALL)
#undef DECL_PSHMEM_WEAK_ALLTOALL
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to generate typed all-to-all collective operations
 * @param _type The C data type
 * @param _typename The type name string
 */

#undef SHMEM_TYPENAME_ALLTOALL
#define SHMEM_TYPENAME_ALLTOALL(_type, _typename)                              \
  int shmem_##_typename##_alltoall(shmem_team_t team, _type *dest,             \
                                   const _type *source, size_t nelems) {       \
    logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %zu)", __func__, team, dest,       \
           source, nelems);                                                    \
    TYPED_CALL(alltoall_type, #_typename, team, dest, source, nelems);         \
  }

#define DECL_SHIM_ALLTOALL(_type, _typename)                                   \
  SHMEM_TYPENAME_ALLTOALL(_type, _typename)
SHMEM_STANDARD_RMA_TYPE_TABLE(DECL_SHIM_ALLTOALL)
#undef DECL_SHIM_ALLTOALL

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

#ifdef ENABLE_PSHMEM
#define DECL_PSHMEM_WEAK_ALLTOALLS(_type, _typename)                           \
  _Pragma("weak shmem_" #_typename "_alltoalls = pshmem_" #_typename           \
          "_alltoalls") #define shmem_##_typename##_alltoalls                  \
      pshmem_##_typename##_alltoalls
SHMEM_STANDARD_RMA_TYPE_TABLE(DECL_PSHMEM_WEAK_ALLTOALLS)
#undef DECL_PSHMEM_WEAK_ALLTOALLS
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to generate typed strided all-to-all collective operations
 * @param _type The C data type
 * @param _typename The type name string
 */
#define SHMEM_TYPENAME_ALLTOALLS(_type, _typename)                             \
  int shmem_##_typename##_alltoalls(shmem_team_t team, _type *dest,            \
                                    const _type *source, ptrdiff_t dst,        \
                                    ptrdiff_t sst, size_t nelems) {            \
    logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %p, %p, %d)", __func__, team,      \
           source, dst, sst, nelems);                                          \
    TYPED_CALL(alltoalls_type, #_typename, team, dest, source, dst, sst,       \
               nelems);                                                        \
  }

#define DECL_SHIM_ALLTOALLS(_type, _typename)                                  \
  SHMEM_TYPENAME_ALLTOALLS(_type, _typename)
SHMEM_STANDARD_RMA_TYPE_TABLE(DECL_SHIM_ALLTOALLS)
#undef DECL_SHIM_ALLTOALLS

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

#ifdef ENABLE_PSHMEM
#define DECL_PSHMEM_WEAK_COLLECT(_type, _typename)                             \
  _Pragma("weak shmem_" #_typename "_collect = pshmem_" #_typename             \
          "_collect") #define shmem_##_typename##_collect                      \
      pshmem_##_typename##_collect
SHMEM_STANDARD_RMA_TYPE_TABLE(DECL_PSHMEM_WEAK_COLLECT)
#undef DECL_PSHMEM_WEAK_COLLECT
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to generate typed collect operations
 * @param _type The C data type
 * @param _typename The type name string
 */
#define SHMEM_TYPENAME_COLLECT(_type, _typename)                               \
  int shmem_##_typename##_collect(shmem_team_t team, _type *dest,              \
                                  const _type *source, size_t nelems) {        \
    logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %d)", __func__, dest, source,      \
           nelems);                                                            \
    TYPED_CALL(collect_type, #_typename, team, dest, source, nelems);          \
  }

#define DECL_SHIM_COLLECT(_type, _typename)                                    \
  SHMEM_TYPENAME_COLLECT(_type, _typename)
SHMEM_STANDARD_RMA_TYPE_TABLE(DECL_SHIM_COLLECT)
#undef DECL_SHIM_COLLECT

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

#ifdef ENABLE_PSHMEM
#define DECL_PSHMEM_WEAK_FCOLLECT(_type, _typename)                            \
  _Pragma("weak shmem_" #_typename "_fcollect = pshmem_" #_typename            \
          "_fcollect") #define shmem_##_typename##_fcollect                    \
      pshmem_##_typename##_fcollect
SHMEM_STANDARD_RMA_TYPE_TABLE(DECL_PSHMEM_WEAK_FCOLLECT)
#undef DECL_PSHMEM_WEAK_FCOLLECT
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to generate typed fixed-length collect operations
 * @param _type The C data type
 * @param _typename The type name string
 */
#define SHMEM_TYPENAME_FCOLLECT(_type, _typename)                              \
  int shmem_##_typename##_fcollect(shmem_team_t team, _type *dest,             \
                                   const _type *source, size_t nelems) {       \
    logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %d)", __func__, dest, source,      \
           nelems);                                                            \
    TYPED_CALL(fcollect_type, #_typename, team, dest, source, nelems);         \
  }

#define DECL_SHIM_FCOLLECT(_type, _typename)                                   \
  SHMEM_TYPENAME_FCOLLECT(_type, _typename)
SHMEM_STANDARD_RMA_TYPE_TABLE(DECL_SHIM_FCOLLECT)
#undef DECL_SHIM_FCOLLECT

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

#ifdef ENABLE_PSHMEM
#define DECL_PSHMEM_WEAK_BROADCAST(_type, _typename)                           \
  _Pragma("weak shmem_" #_typename "_broadcast = pshmem_" #_typename           \
          "_broadcast") #define shmem_##_typename##_broadcast                  \
      pshmem_##_typename##_broadcast
SHMEM_STANDARD_RMA_TYPE_TABLE(DECL_PSHMEM_WEAK_BROADCAST)
#undef DECL_PSHMEM_WEAK_BROADCAST
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to generate typed fixed-length broadcast operations
 * @param _type The C data type
 * @param _typename The type name string
 */
#define SHMEM_TYPENAME_BROADCAST(_type, _typename)                             \
  int shmem_##_typename##_broadcast(shmem_team_t team, _type *dest,            \
                                    const _type *source, size_t nelems,        \
                                    int PE_root) {                             \
    logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %d, %d)", __func__, dest, source,  \
           nelems, PE_root);                                                   \
    TYPED_CALL(broadcast_type, #_typename, team, dest, source, nelems,         \
               PE_root);                                                       \
  }

#define DECL_SHIM_BROADCAST(_type, _typename)                                  \
  SHMEM_TYPENAME_BROADCAST(_type, _typename)
SHMEM_STANDARD_RMA_TYPE_TABLE(DECL_SHIM_BROADCAST)
#undef DECL_SHIM_BROADCAST

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

// #ifdef ENABLE_PSHMEM
// #pragma weak shmem_sync_deprecated = pshmem_sync_deprecated
// #define shmem_sync_deprecated pshmem_sync_deprecated
// #endif /* ENABLE_PSHMEM */

/**
 * @brief Synchronizes a subset of PEs
 *
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
void shmem_sync_deprecated(int PE_start, int logPE_stride, int PE_size,
                           long *pSync) {
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

/**
 * @defgroup to_all To-All Reductions
 * @{
 */

#ifdef ENABLE_PSHMEM
#define DECL_PSHMEM_WEAK_SUM_TO_ALL(_type, _typename)                          \
  _Pragma("weak shmem_" #_typename "_sum_to_all = pshmem_" #_typename          \
          "_sum_to_all") #define shmem_##_typename##_sum_to_all                \
      pshmem_##_typename##_sum_to_all
SHMEM_TOALL_ARITH_TYPE_TABLE(DECL_PSHMEM_WEAK_SUM_TO_ALL)
#undef DECL_PSHMEM_WEAK_SUM_TO_ALL

#define DECL_PSHMEM_WEAK_PROD_TO_ALL(_type, _typename)                         \
  _Pragma("weak shmem_" #_typename "_prod_to_all = pshmem_" #_typename         \
          "_prod_to_all") #define shmem_##_typename##_prod_to_all              \
      pshmem_##_typename##_prod_to_all
SHMEM_TOALL_ARITH_TYPE_TABLE(DECL_PSHMEM_WEAK_PROD_TO_ALL)
#undef DECL_PSHMEM_WEAK_PROD_TO_ALL

#define DECL_PSHMEM_WEAK_AND_TO_ALL(_type, _typename)                          \
  _Pragma("weak shmem_" #_typename "_and_to_all = pshmem_" #_typename          \
          "_and_to_all") #define shmem_##_typename##_and_to_all                \
      pshmem_##_typename##_and_to_all
SHMEM_TOALL_BITWISE_TYPE_TABLE(DECL_PSHMEM_WEAK_AND_TO_ALL)
#undef DECL_PSHMEM_WEAK_AND_TO_ALL

#define DECL_PSHMEM_WEAK_OR_TO_ALL(_type, _typename)                           \
  _Pragma("weak shmem_" #_typename "_or_to_all = pshmem_" #_typename           \
          "_or_to_all") #define shmem_##_typename##_or_to_all                  \
      pshmem_##_typename##_or_to_all
SHMEM_TOALL_BITWISE_TYPE_TABLE(DECL_PSHMEM_WEAK_OR_TO_ALL)
#undef DECL_PSHMEM_WEAK_OR_TO_ALL

#define DECL_PSHMEM_WEAK_XOR_TO_ALL(_type, _typename)                          \
  _Pragma("weak shmem_" #_typename "_xor_to_all = pshmem_" #_typename          \
          "_xor_to_all") #define shmem_##_typename##_xor_to_all                \
      pshmem_##_typename##_xor_to_all
SHMEM_TOALL_BITWISE_TYPE_TABLE(DECL_PSHMEM_WEAK_XOR_TO_ALL)
#undef DECL_PSHMEM_WEAK_XOR_TO_ALL

#define DECL_PSHMEM_WEAK_MAX_TO_ALL(_type, _typename)                          \
  _Pragma("weak shmem_" #_typename "_max_to_all = pshmem_" #_typename          \
          "_max_to_all") #define shmem_##_typename##_max_to_all                \
      pshmem_##_typename##_max_to_all
SHMEM_TOALL_MINMAX_TYPE_TABLE(DECL_PSHMEM_WEAK_MAX_TO_ALL)
#undef DECL_PSHMEM_WEAK_MAX_TO_ALL

#define DECL_PSHMEM_WEAK_MIN_TO_ALL(_type, _typename)                          \
  _Pragma("weak shmem_" #_typename "_min_to_all = pshmem_" #_typename          \
          "_min_to_all") #define shmem_##_typename##_min_to_all                \
      pshmem_##_typename##_min_to_all
SHMEM_TOALL_MINMAX_TYPE_TABLE(DECL_PSHMEM_WEAK_MIN_TO_ALL)
#undef DECL_PSHMEM_WEAK_MIN_TO_ALL
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to generate typed reduction operations with legacy "to_all"
 * naming
 * @param _type The C data type
 * @param _typename The type name string
 * @param _op The reduction operation (sum, prod, and, or, xor, max, min)
 *
 * This macro generates the legacy reduction operations that use the "to_all"
 * naming convention. These operations are deprecated in favor of the newer
 * "_reduce" operations.
 *
 * The generated functions have the following signature:
 * @code
 * void shmem_<typename>_<op>_to_all(_type *target, const _type *source, int
 * nreduce, int PE_start, int logPE_stride, int PE_size, _type *pWrk, long
 * *pSync);
 * @endcode
 *
 * @param target Symmetric destination array
 * @param source Symmetric source array
 * @param nreduce Number of elements in the reduction
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pWrk Symmetric work array
 * @param pSync Symmetric work array for synchronization
 */

#define SHMEM_TYPENAME_TO_ALL(_type, _typename, _op)                           \
  void shmem_##_typename##_##_op##_to_all(                                     \
      _type *target, const _type *source, int nreduce, int PE_start,           \
      int logPE_stride, int PE_size, _type *pWrk, long *pSync) {               \
    logger(LOG_COLLECTIVES, "%s(%p, %p, %d, %d, %d, %p, %p)", __func__,        \
           target, source, nreduce, PE_start, logPE_stride, PE_size, pWrk,     \
           pSync);                                                             \
    TO_ALL_TYPED_CALL(_op##_to_all, #_typename, target, source, nreduce,       \
                      PE_start, logPE_stride, PE_size, pWrk, pSync);           \
  }

#define DECL_SHIM_SUM_TO_ALL(_type, _typename)                                 \
  SHMEM_TYPENAME_TO_ALL(_type, _typename, sum)
SHMEM_TOALL_ARITH_TYPE_TABLE(DECL_SHIM_SUM_TO_ALL)
#undef DECL_SHIM_SUM_TO_ALL

#define DECL_SHIM_PROD_TO_ALL(_type, _typename)                                \
  SHMEM_TYPENAME_TO_ALL(_type, _typename, prod)
SHMEM_TOALL_ARITH_TYPE_TABLE(DECL_SHIM_PROD_TO_ALL)
#undef DECL_SHIM_PROD_TO_ALL

#define DECL_SHIM_AND_TO_ALL(_type, _typename)                                 \
  SHMEM_TYPENAME_TO_ALL(_type, _typename, and)
SHMEM_TOALL_BITWISE_TYPE_TABLE(DECL_SHIM_AND_TO_ALL)
#undef DECL_SHIM_AND_TO_ALL

#define DECL_SHIM_OR_TO_ALL(_type, _typename)                                  \
  SHMEM_TYPENAME_TO_ALL(_type, _typename, or)
SHMEM_TOALL_BITWISE_TYPE_TABLE(DECL_SHIM_OR_TO_ALL)
#undef DECL_SHIM_OR_TO_ALL

#define DECL_SHIM_XOR_TO_ALL(_type, _typename)                                 \
  SHMEM_TYPENAME_TO_ALL(_type, _typename, xor)
SHMEM_TOALL_BITWISE_TYPE_TABLE(DECL_SHIM_XOR_TO_ALL)
#undef DECL_SHIM_XOR_TO_ALL

#define DECL_SHIM_MAX_TO_ALL(_type, _typename)                                 \
  SHMEM_TYPENAME_TO_ALL(_type, _typename, max)
SHMEM_TOALL_MINMAX_TYPE_TABLE(DECL_SHIM_MAX_TO_ALL)
#undef DECL_SHIM_MAX_TO_ALL

#define DECL_SHIM_MIN_TO_ALL(_type, _typename)                                 \
  SHMEM_TYPENAME_TO_ALL(_type, _typename, min)
SHMEM_TOALL_MINMAX_TYPE_TABLE(DECL_SHIM_MIN_TO_ALL)
#undef DECL_SHIM_MIN_TO_ALL

#undef SHMEM_TYPENAME_TO_ALL

/** @} */

/**
 * @defgroup reduce Team Reductions
 * @{
 */

#ifdef ENABLE_PSHMEM
#define DECL_PSHMEM_WEAK_SUM_REDUCE(_type, _typename)                          \
  _Pragma("weak shmem_" #_typename "_sum_reduce = pshmem_" #_typename          \
          "_sum_reduce") #define shmem_##_typename##_sum_reduce                \
      pshmem_##_typename##_sum_reduce
SHMEM_REDUCE_ARITH_TYPE_TABLE(DECL_PSHMEM_WEAK_SUM_REDUCE)
#undef DECL_PSHMEM_WEAK_SUM_REDUCE
#define DECL_PSHMEM_WEAK_PROD_REDUCE(_type, _typename)                         \
  _Pragma("weak shmem_" #_typename "_prod_reduce = pshmem_" #_typename         \
          "_prod_reduce") #define shmem_##_typename##_prod_reduce              \
      pshmem_##_typename##_prod_reduce
SHMEM_REDUCE_ARITH_TYPE_TABLE(DECL_PSHMEM_WEAK_PROD_REDUCE)
#undef DECL_PSHMEM_WEAK_PROD_REDUCE
#define DECL_PSHMEM_WEAK_AND_REDUCE(_type, _typename)                          \
  _Pragma("weak shmem_" #_typename "_and_reduce = pshmem_" #_typename          \
          "_and_reduce") #define shmem_##_typename##_and_reduce                \
      pshmem_##_typename##_and_reduce
SHMEM_REDUCE_BITWISE_TYPE_TABLE(DECL_PSHMEM_WEAK_AND_REDUCE)
#undef DECL_PSHMEM_WEAK_AND_REDUCE
#define DECL_PSHMEM_WEAK_OR_REDUCE(_type, _typename)                           \
  _Pragma("weak shmem_" #_typename "_or_reduce = pshmem_" #_typename           \
          "_or_reduce") #define shmem_##_typename##_or_reduce                  \
      pshmem_##_typename##_or_reduce
SHMEM_REDUCE_BITWISE_TYPE_TABLE(DECL_PSHMEM_WEAK_OR_REDUCE)
#undef DECL_PSHMEM_WEAK_OR_REDUCE
#define DECL_PSHMEM_WEAK_XOR_REDUCE(_type, _typename)                          \
  _Pragma("weak shmem_" #_typename "_xor_reduce = pshmem_" #_typename          \
          "_xor_reduce") #define shmem_##_typename##_xor_reduce                \
      pshmem_##_typename##_xor_reduce
SHMEM_REDUCE_BITWISE_TYPE_TABLE(DECL_PSHMEM_WEAK_XOR_REDUCE)
#undef DECL_PSHMEM_WEAK_XOR_REDUCE
#define DECL_PSHMEM_WEAK_MAX_REDUCE(_type, _typename)                          \
  _Pragma("weak shmem_" #_typename "_max_reduce = pshmem_" #_typename          \
          "_max_reduce") #define shmem_##_typename##_max_reduce                \
      pshmem_##_typename##_max_reduce
SHMEM_REDUCE_MINMAX_TYPE_TABLE(DECL_PSHMEM_WEAK_MAX_REDUCE)
#undef DECL_PSHMEM_WEAK_MAX_REDUCE
#define DECL_PSHMEM_WEAK_MIN_REDUCE(_type, _typename)                          \
  _Pragma("weak shmem_" #_typename "_min_reduce = pshmem_" #_typename          \
          "_min_reduce") #define shmem_##_typename##_min_reduce                \
      pshmem_##_typename##_min_reduce
SHMEM_REDUCE_MINMAX_TYPE_TABLE(DECL_PSHMEM_WEAK_MIN_REDUCE)
#undef DECL_PSHMEM_WEAK_MIN_REDUCE
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to generate typed reduction operations
 * @param _type The C data type
 * @param _typename The type name string
 * @param _op The reduction operation (sum, prod, and, or, xor, max, min)
 *
 * This macro generates the function definitions for typed reduction operations.
 * It creates functions of the form shmem_<typename>_<op>_reduce() that perform
 * the specified reduction operation on the given data type.
 *
 * The generated functions take the following parameters:
 * @param team The team over which to perform the reduction
 * @param dest The destination array for the result
 * @param source The source array containing input values
 * @param nreduce Number of elements to reduce
 * @return Zero on success, non-zero on failure
 */

#define SHMEM_TYPENAME_REDUCE(_type, _typename, _op)                           \
  int shmem_##_typename##_##_op##_reduce(                                      \
      shmem_team_t team, _type *dest, const _type *source, size_t nreduce) {   \
    logger(LOG_COLLECTIVES, "%s(%p, %p, %p, %zu)", __func__, team, dest,       \
           source, nreduce);                                                   \
    TYPED_CALL(_op##_reduce, #_typename, team, dest, source, nreduce);         \
  }

#define DECL_SHIM_SUM_REDUCE(_type, _typename)                                 \
  SHMEM_TYPENAME_REDUCE(_type, _typename, sum)
SHMEM_REDUCE_ARITH_TYPE_TABLE(DECL_SHIM_SUM_REDUCE)
#undef DECL_SHIM_SUM_REDUCE
#define DECL_SHIM_PROD_REDUCE(_type, _typename)                                \
  SHMEM_TYPENAME_REDUCE(_type, _typename, prod)
SHMEM_REDUCE_ARITH_TYPE_TABLE(DECL_SHIM_PROD_REDUCE)
#undef DECL_SHIM_PROD_REDUCE
#define DECL_SHIM_AND_REDUCE(_type, _typename)                                 \
  SHMEM_TYPENAME_REDUCE(_type, _typename, and)
SHMEM_REDUCE_BITWISE_TYPE_TABLE(DECL_SHIM_AND_REDUCE)
#undef DECL_SHIM_AND_REDUCE
#define DECL_SHIM_OR_REDUCE(_type, _typename)                                  \
  SHMEM_TYPENAME_REDUCE(_type, _typename, or)
SHMEM_REDUCE_BITWISE_TYPE_TABLE(DECL_SHIM_OR_REDUCE)
#undef DECL_SHIM_OR_REDUCE
#define DECL_SHIM_XOR_REDUCE(_type, _typename)                                 \
  SHMEM_TYPENAME_REDUCE(_type, _typename, xor)
SHMEM_REDUCE_BITWISE_TYPE_TABLE(DECL_SHIM_XOR_REDUCE)
#undef DECL_SHIM_XOR_REDUCE
#define DECL_SHIM_MAX_REDUCE(_type, _typename)                                 \
  SHMEM_TYPENAME_REDUCE(_type, _typename, max)
SHMEM_REDUCE_MINMAX_TYPE_TABLE(DECL_SHIM_MAX_REDUCE)
#undef DECL_SHIM_MAX_REDUCE
#define DECL_SHIM_MIN_REDUCE(_type, _typename)                                 \
  SHMEM_TYPENAME_REDUCE(_type, _typename, min)
SHMEM_REDUCE_MINMAX_TYPE_TABLE(DECL_SHIM_MIN_REDUCE)
#undef DECL_SHIM_MIN_REDUCE
#undef SHMEM_TYPENAME_REDUCE

/** @} */
