/**
 * @file memcheck.h
 * @brief Header for memory corruption checking functions
 *
 * For license: see LICENSE file at top-level
 */

#ifndef _MEMCHECK_H
#define _MEMCHECK_H 1

/*
 * This is pulled into internal-malloc.c:2519 to define our customized
 * handlers, which prevents implicit declaration warnings.
 */

/**
 * @brief Reports memory corruption detected in a memory space
 *
 * @param m Memory space handle where corruption was detected
 */
extern void report_corruption(mspace m);

/**
 * @brief Reports improper usage of memory allocation functions
 *
 * @param m Memory space handle where error occurred
 * @param p Pointer involved in the usage error
 */
extern void report_usage_error(mspace m, void *p);

#endif /* ! _MEMCHECK_H */
