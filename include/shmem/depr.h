/* For license: see LICENSE file at top-level */

/**
 * @file depr.h
 * @brief Macros and definitions for handling deprecated OpenSHMEM functionality
 */

#ifndef _SHMEM_DEPR_H
#define _SHMEM_DEPR_H 1

#ifdef _SHMEM_IN_SOURCE_TREE_

/**
 * @brief Within source tree, deprecation warnings are disabled
 */
#define _DEPRECATED_BY(...)
#define _DEPRECATED

#else

/**
 * @brief Helper macro to create deprecation message with replacement suggestion
 * @param _text The suggested replacement API
 * @param _version The specification version where deprecation occurred
 */
#define _ADORNED_DEPRECATION(_text, _version)                                  \
  __attribute__((deprecated("with specification " #_version                    \
                            " and up, use `" #_text "' instead")))

/*
 * TODO: need better detection
 */
#if defined(__clang__)

/**
 * @brief Clang-specific deprecation macros
 */
#define _DEPRECATED_BY(_text, _version) _ADORNED_DEPRECATION(_text, _version)
#define _DEPRECATED __attribute__((deprecated))

#elif defined(__OPEN64__)

/* not supported */

/**
 * @brief Open64 compiler does not support deprecation attributes
 */
#define _DEPRECATED_BY(...)
#define _DEPRECATED

#elif defined(__GNUC__)

/**
 * @brief Basic GCC deprecation attribute
 */
#define _DEPRECATED __attribute__((deprecated))

/* GCC has extended attribute syntax from 4.5 onward */

#if (__GNUC__ >= 5) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
/**
 * @brief Enhanced GCC deprecation with message (GCC >= 4.5)
 */
#define _DEPRECATED_BY(_text, _version) _ADORNED_DEPRECATION(_text, _version)
#else
/**
 * @brief Basic deprecation for older GCC versions
 */
#define _DEPRECATED_BY(...) _DEPRECATED
#endif

#else

/* fallback */

/**
 * @brief Fallback deprecation macros for unsupported compilers
 */
#define _DEPRECATED_BY(...)
#define _DEPRECATED

#endif /* compiler deprecation check */

#endif /* _SHMEM_IN_SOURCE_TREE_ */

#endif /* ! _SHMEM_DEPR_H */
