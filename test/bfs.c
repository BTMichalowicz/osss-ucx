#include "../build/install/include/shmem.h"
#include "../build/install/include/shmem/defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Maximum neighbors per vertex (for simplicity)
#define MAX_NEIGHBORS 4

// Generate a synthetic graph (random sparse graph)
void generate_graph(int *adj_list, int local_n, int global_n, int mype,
                    int npes) {
  srand(mype + 1); // Different seed per PE
  for (int i = 0; i < local_n; i++) {
    int v = mype * local_n + i; // Global vertex ID
    int num_neighbors =
        rand() % MAX_NEIGHBORS + 1; // 1 to MAX_NEIGHBORS neighbors
    for (int j = 0; j < num_neighbors; j++) {
      // Random neighbor (ensure within graph)
      int neighbor = rand() % global_n;
      // Avoid self-loops
      if (neighbor != v) {
        adj_list[i * MAX_NEIGHBORS + j] = neighbor;
      } else {
        adj_list[i * MAX_NEIGHBORS + j] = -1; // Invalid neighbor
      }
    }
    // Fill remaining slots with -1
    for (int j = num_neighbors; j < MAX_NEIGHBORS; j++) {
      adj_list[i * MAX_NEIGHBORS + j] = -1;
    }
  }
}

int main(int argc, char *argv[]) {
  // Initialize OpenSHMEM
  shmem_init();
  int mype = shmem_my_pe();
  int npes = shmem_n_pes();

  // Print OpenSHMEM version for verification
  if (mype == 0) {
    int major, minor;
    shmem_info_get_version(&major, &minor);
    printf("OpenSHMEM version: %d.%d\n", major, minor);
  }

  // Graph size (number of vertices)
  int global_n = 1024; // Adjustable
  if (argc > 1) {
    global_n = atoi(argv[1]);
  }
  if (global_n % npes != 0) {
    if (mype == 0) {
      printf("Error: Number of vertices (%d) must be divisible by number of "
             "PEs (%d).\n",
             global_n, npes);
    }
    shmem_finalize();
    return 1;
  }
  int local_n = global_n / npes; // Vertices per PE

  // Allocate symmetric memory
  int *adj_list =
      shmem_malloc(local_n * MAX_NEIGHBORS * sizeof(int));   // Adjacency list
  int *local_frontier = shmem_malloc(local_n * sizeof(int)); // Local frontier
  int *next_frontier = shmem_malloc(local_n * sizeof(int));  // Next frontier
  int *visited = shmem_malloc(local_n * sizeof(int));        // Visited flags
  int *parent = shmem_malloc(local_n * sizeof(int));         // BFS parent array
  // Allocate work and sync arrays for reduction
  int *pWrk = shmem_malloc(sizeof(int) * (1)); // Work array for 1 element
  long *pSync =
      shmem_malloc(SHMEM_REDUCE_SYNC_SIZE * sizeof(long)); // Sync array

  if (!adj_list || !local_frontier || !next_frontier || !visited || !parent ||
      !pWrk || !pSync) {
    if (mype == 0) {
      printf("Error: Memory allocation failed.\n");
    }
    shmem_finalize();
    return 1;
  }

  // Initialize data
  for (int i = 0; i < local_n; i++) {
    local_frontier[i] = 0;
    next_frontier[i] = 0;
    visited[i] = 0;
    parent[i] = -1;
  }
  // Initialize pSync for reduction
  for (int i = 0; i < SHMEM_REDUCE_SYNC_SIZE; i++) {
    pSync[i] = SHMEM_SYNC_VALUE;
  }
  generate_graph(adj_list, local_n, global_n, mype, npes);

  // Root vertex (vertex 0, owned by PE 0)
  if (mype == 0) {
    local_frontier[0] = 1; // Mark vertex 0 as in frontier
    visited[0] = 1;        // Mark as visited
    parent[0] = 0;         // Root is its own parent
  }

  shmem_barrier_all();

  // BFS loop
  int done = 0;
  while (!done) {
    // Clear next frontier
    for (int i = 0; i < local_n; i++) {
      next_frontier[i] = 0;
    }

    // Process local frontier
    for (int i = 0; i < local_n; i++) {
      if (local_frontier[i]) {
        int v = mype * local_n + i; // Global vertex ID
        // Explore neighbors
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
          int u = adj_list[i * MAX_NEIGHBORS + j];
          if (u >= 0 && u < global_n) { // Valid neighbor
            int owner_pe = u / local_n; // PE owning vertex u
            int local_u = u % local_n;  // Local index on owner_pe
            // Check if unvisited (use get to avoid races)
            int u_visited;
            shmem_get(&u_visited, &visited[local_u], sizeof(int), owner_pe);
            if (!u_visited) {
              // Mark as visited and set parent
              shmem_put(&visited[local_u], &(int){1}, sizeof(int), owner_pe);
              shmem_put(&parent[local_u], &v, sizeof(int), owner_pe);
              // Add to next frontier
              shmem_put(&next_frontier[local_u], &(int){1}, sizeof(int),
                        owner_pe);
            }
          }
        }
      }
    }

    shmem_quiet(); // Ensure all puts complete
    shmem_barrier_all();

    // Swap frontiers
    for (int i = 0; i < local_n; i++) {
      local_frontier[i] = next_frontier[i];
    }

    // Check if frontier is empty (global reduction)
    int local_empty = 1;
    for (int i = 0; i < local_n; i++) {
      if (local_frontier[i]) {
        local_empty = 0;
        break;
      }
    }
    int global_empty;
    shmem_int_or_reduce(SHMEM_TEAM_WORLD, &global_empty, &local_empty, 1, pWrk,
                        pSync);
    done = global_empty;
    shmem_barrier_all();
  }

  // Print sample results from PE 0
  if (mype == 0) {
    printf("BFS parent array (first 5 vertices):\n");
    for (int i = 0; i < 5 && i < local_n; i++) {
      printf("Vertex %d: parent = %d\n", i, parent[i]);
    }
  }

  // Cleanup
  shmem_free(adj_list);
  shmem_free(local_frontier);
  shmem_free(next_frontier);
  shmem_free(visited);
  shmem_free(parent);
  shmem_free(pWrk);
  shmem_free(pSync);

  shmem_finalize();
  return 0;
}