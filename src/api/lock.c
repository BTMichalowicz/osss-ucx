/* For license: see LICENSE file at top-level */

/*
 * Rewrite of an original MCS lock code by
 *
 *    Copyright (c) 1996-2002 by Quadrics Supercomputers World Ltd.
 *    Copyright (c) 2003-2005 by Quadrics Ltd.
 */

/**
 * @file lock.c
 * @brief Implementation of OpenSHMEM distributed locking routines
 *
 * This file contains implementations of distributed locking operations that
 * provide mutual exclusion across PEs. The implementation is based on the
 * MCS lock algorithm.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"
#include "shmemc.h"
#include "shmem.h"
#include "shmem_mutex.h"

#include <stdint.h>
#include <sys/types.h>

/*
 * this overlays an opaque blob we can move around with AMOs, and the
 * signalling/PE parts.
 *
 * TODO "short" PE in this setup, should be "int"
 */

enum {
  SHMEM_LOCK_FREE = -1,
  SHMEM_LOCK_RESET = 0, /* value matches lock initializer in spec */
  SHMEM_LOCK_ACQUIRED
};

/**
 * @brief Lock data structure that combines lock state and next PE
 *
 * This union allows the lock to be manipulated both as separate fields
 * and as a single atomic value.
 */
typedef union shmem_lock {
  struct data_split {
    int16_t locked; /**< Lock state */
    int16_t next;   /**< Next PE in lock queue */
  } d;
  int32_t blob; /**< Combined value for atomic operations */
} shmem_lock_t;

/*
 * spread lock ownership around PEs
 */

/**
 * @brief Calculate lock owner PE based on address
 *
 * @param addr Address of the lock
 * @return PE number that should own this lock
 */
inline static int get_owner_spread(uint64_t addr) {
  return (addr >> 3) % shmemc_n_pes();
}

/**
 * @brief Determine the owner PE for a lock
 *
 * @param addr Address of the lock
 * @return PE number that owns this lock
 */
inline static int lock_owner(void *addr) {
  const uint64_t la = (const uint64_t)addr;
  int owner;

  /*
   * can only agree on distributed owners if we all agree on aligned
   * addresses
   */
#ifdef ENABLE_ALIGNED_ADDRESSES
  owner = get_owner_spread(la);
#else
  if (shmemc_global_address(la)) {
    owner = get_owner_spread(la);
  } else {
    /* don't choose PE 0, as it is often used for work allocation */
    owner = shmemc_n_pes() - 1;
  }
#endif /* ENABLE_ALIGNED_ADDRESSES */

  return owner;
}

/*
 * split the lock claim into 2-phase request + execute.
 *
 * "cmp" contains the claim and connects the 2 phases
 */

/**
 * @brief Common atomic lock operation
 *
 * @param lock Lock to operate on
 * @param cond Expected value for compare-swap
 * @param value New value to swap in
 * @param cmp Result of operation
 */
inline static void try_lock_action(shmem_lock_t *lock, int cond, int value,
                                   shmem_lock_t *cmp) {
  cmp->blob = shmem_int_atomic_compare_swap(&(lock->blob), cond, value,
                                            lock_owner(lock));
}

/**
 * @brief Attempt to request a lock
 *
 * @param lock Lock to request
 * @param me Current PE
 * @param cmp Operation result
 */
inline static void try_request_lock(shmem_lock_t *lock, int me,
                                    shmem_lock_t *cmp) {
  const shmem_lock_t tmp = {.d.locked = SHMEM_LOCK_ACQUIRED, .d.next = me};

  try_lock_action(lock, SHMEM_LOCK_RESET, tmp.blob, cmp);
}

/**
 * @brief Attempt to clear a lock
 *
 * @param lock Lock to clear
 * @param me Current PE
 * @param cmp Operation result
 */
inline static void try_clear_lock(shmem_lock_t *lock, int me,
                                  shmem_lock_t *cmp) {
  const shmem_lock_t tmp = {.d.locked = SHMEM_LOCK_ACQUIRED, .d.next = me};

  try_lock_action(lock, tmp.blob, SHMEM_LOCK_RESET, cmp);
}

/**
 * @brief Request phase for set_lock operation
 *
 * @param lock Lock to set
 * @param me Current PE
 * @param cmp Operation result
 */
inline static void set_lock_request(shmem_lock_t *lock, int me,
                                    shmem_lock_t *cmp) {
  /* push my claim into the owner */
  do {
    try_request_lock(lock, me, cmp);
  } while (cmp->blob != SHMEM_LOCK_RESET);
}

/**
 * @brief Request phase for test_lock operation
 *
 * @param lock Lock to test
 * @param me Current PE
 * @param cmp Operation result
 */
inline static void test_lock_request(shmem_lock_t *lock, int me,
                                     shmem_lock_t *cmp) {
  /* if owner is unset, grab the lock */
  try_request_lock(lock, me, cmp);
}

/**
 * @brief Request phase for clear_lock operation
 *
 * @param node Local lock data
 * @param lock Lock to clear
 * @param me Current PE
 * @param cmp Operation result
 */
inline static void clear_lock_request(shmem_lock_t *node, shmem_lock_t *lock,
                                      int me, shmem_lock_t *cmp) {
  if (node->d.next == SHMEM_LOCK_FREE) {
    try_clear_lock(lock, me, cmp);
  }
}

/**
 * @brief Execute phase for set_lock operation
 *
 * @param node Local lock data
 * @param me Current PE
 * @param cmp Operation result
 */
inline static void set_lock_execute(shmem_lock_t *node, int me,
                                    shmem_lock_t *cmp) {
  /* tail */
  node->d.next = SHMEM_LOCK_FREE;

  if (cmp->d.locked == SHMEM_LOCK_ACQUIRED) {
    node->d.locked = SHMEM_LOCK_ACQUIRED;

    /* chain me on */
    shmem_short_p(&(node->d.next), me, cmp->d.next);

    /* sit here until unlocked */
    do {
      shmemc_progress();
    } while (node->d.locked == SHMEM_LOCK_ACQUIRED);
  }
}

/**
 * @brief Execute phase for test_lock operation
 *
 * @param node Local lock data
 * @param me Current PE
 * @param cmp Operation result
 * @return 0 on success, 1 if lock not acquired
 */
inline static int test_lock_execute(shmem_lock_t *node, int me,
                                    shmem_lock_t *cmp) {
  if (cmp->blob == SHMEM_LOCK_RESET) {
    /* grabbed unset lock, now go on to set the rest of the lock */
    set_lock_execute(node, me, cmp);
    return 0;
  } else {
    /* nope, go around again */
    return 1;
  }
}

/**
 * @brief Execute phase for clear_lock operation
 *
 * @param node Local lock data
 * @param me Current PE
 * @param cmp Operation result
 */
inline static void clear_lock_execute(shmem_lock_t *node, int me,
                                      shmem_lock_t *cmp) {
  /* any more chainers? */
  if (cmp->d.next == me) {
    return;
    /* NOT REACHED */
  }

  /* wait for a chainer PE to appear */
  do {
    shmemc_progress();
  } while (node->d.next == SHMEM_LOCK_FREE);

  /* tell next pe about release */
  shmem_short_p(&(node->d.locked), SHMEM_LOCK_RESET, node->d.next);
}

/**
 * @brief Internal blocking set_lock implementation
 *
 * @param node Local lock data
 * @param lock Lock to set
 * @param me Current PE
 */
inline static void set_lock(shmem_lock_t *node, shmem_lock_t *lock, int me) {
  shmem_lock_t t;

  set_lock_request(lock, me, &t);
  set_lock_execute(node, me, &t);
}

/**
 * @brief Internal blocking clear_lock implementation
 *
 * @param node Local lock data
 * @param lock Lock to clear
 * @param me Current PE
 */
inline static void clear_lock(shmem_lock_t *node, shmem_lock_t *lock, int me) {
  shmem_lock_t t;

  /* required to flush comms before clearing lock */
  shmemc_quiet();

  clear_lock_request(node, lock, me, &t);
  clear_lock_execute(node, me, &t);
}

/**
 * @brief Internal blocking test_lock implementation
 *
 * @param node Local lock data
 * @param lock Lock to test
 * @param me Current PE
 * @return 0 on success, non-zero if lock not acquired
 */
inline static int test_lock(shmem_lock_t *node, shmem_lock_t *lock, int me) {
  shmem_lock_t t;

  test_lock_request(lock, me, &t);
  return test_lock_execute(node, me, &t);
}

/**
 * API
 */

#ifdef ENABLE_PSHMEM
#pragma weak shmem_set_lock = pshmem_set_lock
#define shmem_set_lock pshmem_set_lock
#pragma weak shmem_test_lock = pshmem_test_lock
#define shmem_test_lock pshmem_test_lock
#pragma weak shmem_clear_lock = pshmem_clear_lock
#define shmem_clear_lock pshmem_clear_lock
#endif /* ENABLE_PSHMEM */

/*
 * split the "big" user-visible lock into the internal management
 * types
 */

#define UNPACK()                                                               \
  shmem_lock_t *base = (shmem_lock_t *)lp;                                     \
  shmem_lock_t *node = base + 1;                                               \
  shmem_lock_t *lock = base + 0

/**
 * @brief Set (acquire) a distributed lock
 *
 * @param lp Pointer to the lock
 *
 * Blocks until the lock is acquired. Multiple PEs calling this routine
 * will be queued in order of arrival.
 */
void shmem_set_lock(long *lp) {
  UNPACK();

  SHMEMU_CHECK_INIT();
  SHMEMU_CHECK_NOT_NULL(lp, 1);
  SHMEMU_CHECK_SYMMETRIC(lp, 1);

  logger(LOG_LOCKS, "%s(lock=%p)", __func__, lock);

  SHMEMT_MUTEX_NOPROTECT(set_lock(node, lock, shmemc_my_pe()));
}

/**
 * @brief Release a distributed lock
 *
 * @param lp Pointer to the lock
 *
 * Releases a lock previously acquired by shmem_set_lock. If any other PEs
 * are waiting for the lock, the first in line will acquire it.
 */
void shmem_clear_lock(long *lp) {
  UNPACK();

  SHMEMU_CHECK_INIT();
  SHMEMU_CHECK_NOT_NULL(lp, 1);
  SHMEMU_CHECK_SYMMETRIC(lp, 1);

  logger(LOG_LOCKS, "%s(lock=%p)", __func__, lock);

  SHMEMT_MUTEX_NOPROTECT(clear_lock(node, lock, shmemc_my_pe()));
}

/**
 * @brief Attempt to acquire a distributed lock
 *
 * @param lp Pointer to the lock
 * @return 0 if lock acquired, non-zero otherwise
 *
 * Non-blocking attempt to acquire a lock. Returns immediately if lock
 * cannot be acquired.
 */
int shmem_test_lock(long *lp) {
  int ret;
  UNPACK();

  SHMEMU_CHECK_INIT();
  SHMEMU_CHECK_NOT_NULL(lp, 1);
  SHMEMU_CHECK_SYMMETRIC(lp, 1);

  logger(LOG_LOCKS, "%s(lock=%p)", __func__, lock);

  SHMEMT_MUTEX_NOPROTECT(ret = test_lock(node, lock, shmemc_my_pe()));

  return ret;
}
