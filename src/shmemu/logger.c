/**
 * @file logger.c
 * @brief Logging functionality for OpenSHMEM
 *
 * This file implements a configurable logging system for OpenSHMEM that
 * supports:
 * - Multiple logging destinations (file or stderr)
 * - Selective event filtering
 * - Timestamped messages with PE/process info
 * - Dynamic log file naming with format specifiers
 *
 * @copyright See LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "state.h"
#include "shmemu.h"
#include "shmemc.h"
#include "boolean.h"
#include "module.h"

#include "../klib/khash.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/time.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

/** File handle for log output */
static FILE *log_stream = NULL;

/**
 * Output formatting widths
 */
static int pe_width;    /**< Width for PE number display */
static int stamp_width; /**< Width for timestamp display */

/** Cached process ID */
static pid_t mypid;

/**
 * Event tracking hash table
 */
KHASH_MAP_INIT_STR(events_hash, bool)

/** Hash table to track enabled logging events */
static khash_t(events_hash) * events;

/**
 * @brief Set the enabled state for a logging event
 *
 * @param name Name of the logging event
 * @param state True to enable, false to disable
 */
static void event_set(shmemu_log_t name, bool state) {
  khiter_t k;
  int ret;

  k = kh_put(events_hash, events, name, &ret);
  kh_value(events, k) = state;
}

/**
 * @brief Check if a logging event is enabled
 *
 * @param name Name of the logging event
 * @return true if event is enabled, false otherwise
 */
static bool event_enabled(shmemu_log_t name) {
  const khiter_t k = kh_get(events_hash, events, name);

  return (k != kh_end(events)) ? kh_value(events, k) : false;
}

/**
 * @brief Convert string to uppercase in-place
 *
 * @param str String to convert
 */
static void upperize(char *str) {
  char *q;

  for (q = str; *q != '\0'; ++q) {
    *q = toupper(*q);
  }
}

/**
 * @brief Parse logging events from environment variable
 *
 * Parses comma/semicolon separated list of events to enable
 */
static void parse_log_events(void) {
  const char *delims = ",:;";
  char *cp = proc.env.logging_events;
  char *opt;

  if (cp == NULL) {
    return;
    /* NOT REACHED */
  }

  opt = strtok(cp, delims);

  while (opt != NULL) {
    upperize(opt);
    event_set(opt, true);
    opt = strtok(NULL, delims);
  }
}

/**
 * Format specifiers for log file names:
 * %p - process ID
 * %h - host name
 * %n - my rank/PE
 * %N - number of ranks/PEs
 */

#define REPLACE(_fmt, _val)                                                    \
  do {                                                                         \
    ++p;                                                                       \
    snprintf(lp, len, _fmt, _val);                                             \
    while (*lp != '\0') {                                                      \
      ++lp;                                                                    \
    }                                                                          \
  } while (0)

/**
 * @brief Parse log file name template and substitute format specifiers
 *
 * @param name Buffer to store generated filename
 * @param len Length of name buffer
 * @param template Template string containing format specifiers
 */
static void parse_logfile_name(char *name, size_t len, const char *template) {
  const char format_character = '%';
  char *p = (char *)template;
  char *lp = name;

  while (*p != '\0') {
    if (*p == format_character) { /* potential format, check next char */
      ++p;

      if (*p == '\0') { /* literal copy, stop */
        *lp++ = format_character;
        break;
      }

      switch (*p) {
      case 'p':
        REPLACE("%d", mypid);
        break;
      case 'h':
        REPLACE("%s", proc.nodename);
        break;
      case 'n':
        REPLACE("%d", shmemc_my_pe());
        break;
      case 'N':
        REPLACE("%d", shmemc_n_pes());
        break;
      default: /* not format, so just copy */
        *lp++ = format_character;
        *lp++ = *p++;
        break;
      }
    } else { /* literal copy */
      *lp++ = *p++;
    }
  }
  *lp = '\0';
}

/**
 * @brief Initialize the logging system
 *
 * Sets up log file, formatting parameters, and default event states.
 * Parses environment variables to configure logging behavior.
 */
void shmemu_logger_init(void) {
  mypid = getpid();

  if (proc.env.logging) {

    if (proc.env.logging_file != NULL) {
      char lfname[PATH_MAX];

      parse_logfile_name((char *)lfname, PATH_MAX, proc.env.logging_file);
      log_stream = fopen(lfname, "w");
      if (log_stream == NULL) {
        shmemu_fatal(MODULE ": can't write to log file \"%s\"",
                     proc.env.logging_file);
        /* NOT REACHED */
      }
    } else {
      log_stream = stderr;
    }

    /* how wide to display things */
    pe_width = (int)ceil(log10((double)proc.li.nranks));
    stamp_width = 30 - pe_width;
    if (stamp_width < 1) {
      stamp_width = 1;
    }

    events = kh_init(events_hash);

    event_set(LOG_INIT, false);
    event_set(LOG_FINALIZE, false);
    event_set(LOG_MEMORY, false);
    event_set(LOG_FENCE, false);
    event_set(LOG_QUIET, false);
    event_set(LOG_HEAPS, false);
    event_set(LOG_RMA, false);
    event_set(LOG_CONTEXTS, false);
    event_set(LOG_RANKS, false);
    event_set(LOG_INFO, false);
    event_set(LOG_REDUCTIONS, false);
    event_set(LOG_COLLECTIVES, false);
    event_set(LOG_DEPRECATE, false);
    event_set(LOG_LOCKS, false);
    event_set(LOG_ATOMICS, false);

    parse_log_events();
  }
}

/**
 * @brief Clean up the logging system
 *
 * Closes log file and frees event tracking resources
 */
void shmemu_logger_finalize(void) {
  if (proc.env.logging) {
    fclose(log_stream);

    kh_destroy(events_hash, events);
  }
}

/** Buffer sizes for log message formatting */
#define TRACE_MSG_BUF_SIZE_1 256
#define TRACE_MSG_BUF_SIZE_2 (TRACE_MSG_BUF_SIZE_1 * 2)

/** Temporary buffers for message formatting */
static char tmp1[TRACE_MSG_BUF_SIZE_1];
static char tmp2[TRACE_MSG_BUF_SIZE_2];

/**
 * @brief Log a message if the event type is enabled
 *
 * Formats and outputs a log message with timestamp, PE info, and event type
 * if the specified event type is enabled.
 *
 * @param evt Event type for this message
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 */
void shmemu_logger(shmemu_log_t evt, const char *fmt, ...) {
  if (proc.env.logging) {

    if (event_enabled(evt) || event_enabled(LOG_ALL)) {
      va_list ap;

      snprintf(tmp1, TRACE_MSG_BUF_SIZE_1, "[%*d:%s:%d:%6.6f]", pe_width,
               proc.li.rank, proc.nodename, mypid, shmemu_timer());

      snprintf(tmp2, TRACE_MSG_BUF_SIZE_2, "%-*s %s: ", stamp_width, tmp1, evt);

      va_start(ap, fmt);
      vsnprintf(tmp1, TRACE_MSG_BUF_SIZE_1, fmt, ap);
      va_end(ap);

      STRNCAT_SAFE(tmp2, tmp1, TRACE_MSG_BUF_SIZE_1);
      STRNCAT_SAFE(tmp2, "\n", 2);

      fputs(tmp2, log_stream);
      /* make sure this all goes out in 1 burst */
      fflush(log_stream);
    }
  }
}
