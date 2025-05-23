/* For license: see LICENSE file at top-level */

/**
 * @file info.h
 * @brief Header file for OpenSHMEM information output routines
 *
 * This file contains declarations of routines that output information about
 * the OpenSHMEM library configuration, build environment, and features.
 */

#ifndef _SHMEM_OSH_INFO_H
#define _SHMEM_OSH_INFO_H 1

#include <stdio.h>

/**
 * @brief Output OpenSHMEM specification version information
 *
 * @param strm Output stream to write to
 * @param prefix String to prepend to output
 * @param suffix String to append to output
 */
void info_output_spec_version(FILE *strm, const char *prefix,
                              const char *suffix);

/**
 * @brief Output OpenSHMEM package name information
 *
 * @param strm Output stream to write to
 * @param prefix String to prepend to output
 * @param suffix String to append to output
 */
void info_output_package_name(FILE *strm, const char *prefix,
                              const char *suffix);

/**
 * @brief Output OpenSHMEM package contact information
 *
 * @param strm Output stream to write to
 * @param prefix String to prepend to output
 * @param suffix String to append to output
 */
void info_output_package_contact(FILE *strm, const char *prefix,
                                 const char *suffix);

/**
 * @brief Output OpenSHMEM package version information
 *
 * @param strm Output stream to write to
 * @param prefix String to prepend to output
 * @param suffix String to append to output
 * @param terse If non-zero, output in terse format
 */
void info_output_package_version(FILE *strm, const char *prefix,
                                 const char *suffix, int terse);

/**
 * @brief Output OpenSHMEM build environment information
 *
 * @param strm Output stream to write to
 * @param prefix String to prepend to output
 * @param suffix String to append to output
 */
void info_output_build_env(FILE *strm, const char *prefix, const char *suffix);

/**
 * @brief Output OpenSHMEM features information
 *
 * @param strm Output stream to write to
 * @param prefix String to prepend to output
 * @param suffix String to append to output
 */
void info_output_features(FILE *strm, const char *prefix, const char *suffix);

/**
 * @brief Output OpenSHMEM communications layer information
 *
 * @param strm Output stream to write to
 * @param prefix String to prepend to output
 * @param suffix String to append to output
 */
void info_output_comms(FILE *strm, const char *prefix, const char *suffix);

/**
 * @brief Output OpenSHMEM help information
 *
 * @param strm Output stream to write to
 * @param prefix String to prepend to output
 * @param suffix String to append to output
 */
void info_output_help(FILE *strm, const char *prefix, const char *suffix);

#endif /* ! _SHMEM_OSH_INFO_H */
