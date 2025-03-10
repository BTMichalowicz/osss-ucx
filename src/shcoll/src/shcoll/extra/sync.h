#ifndef _SHCOLL_SYNC_H
#define _SHCOLL_SYNC_H 1

#include "shmem/teams.h"

void shcoll_set_tree_degree(int tree_degree);
void shcoll_set_knomial_tree_radix_barrier(int tree_radix);

#define SCHOLL_SYNC_DECLARATION(_name)                                         \
  int shcoll_sync_##_name(shmem_team_t team);                                  \
                                                                               \
  void shcoll_sync_all_##_name(long *pSync);

SCHOLL_SYNC_DECLARATION(linear)
SCHOLL_SYNC_DECLARATION(complete_tree)
SCHOLL_SYNC_DECLARATION(binomial_tree)
SCHOLL_SYNC_DECLARATION(knomial_tree)
SCHOLL_SYNC_DECLARATION(dissemination)

#endif /* ! _SHCOLL_SYNC_H */
