/**
 * @file pmi_client.h
 * @brief PMI client interface for OpenSHMEM communications layer
 *
 * This header defines the Process Management Interface (PMI) client functions
 * used by OpenSHMEM for process coordination, barrier operations, and data
 * exchange.
 *
 * @copyright See LICENSE file at top-level
 */

#ifndef _SHMEMC_PMI_CLIENT_H
#define _SHMEMC_PMI_CLIENT_H 1

/**
 * @brief Initialize the PMI client interface
 */
void shmemc_pmi_client_init(void);

/**
 * @brief Finalize and cleanup the PMI client interface
 */
void shmemc_pmi_client_finalize(void);

/**
 * @brief Abort execution with an error message
 *
 * @param msg Error message to display
 * @param status Exit status code
 */
void shmemc_pmi_client_abort(const char *msg, int status);

/**
 * @brief Perform a barrier operation across all processes
 *
 * @param collect_data Whether to collect data during barrier
 */
void shmemc_pmi_barrier_all(bool collect_data);

/**
 * @brief Publish worker information to other processes
 */
void shmemc_pmi_publish_worker(void);

/**
 * @brief Publish remote keys and heap information
 */
void shmemc_pmi_publish_rkeys_and_heaps(void);

/**
 * @brief Exchange worker information between processes
 */
void shmemc_pmi_exchange_workers(void);

/**
 * @brief Exchange remote keys and heap information between processes
 */
void shmemc_pmi_exchange_rkeys_and_heaps(void);

#endif /* ! _SHMEMC_PMI_CLIENT_H */
