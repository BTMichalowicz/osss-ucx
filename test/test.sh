#!/bin/bash

################################################################
#                          SETUP
################################################################
# --- Set UCX configuration
export UCX_TLS=sm
export UCX_RC_DEVX_ENABLED=n
# export UCX_LOG_LEVEL=debug

# --- Turn on verbose SHMEM logging
# export SHMEM_DEBUG=on

# --- Set OSSS bins from build directory
oshcc="../build/install/bin/oshcc"
oshrun="../build/install/bin/oshrun"

# --- Clean bin directory
rm -rf bin
mkdir bin

# --- Set flags for mpiexec
flags="--bind-to core --map-by core --allow-run-as-root"
hline="-------------------------------"

# --- Print log levels ---
export PMIX_MCA_pcompress_base_silence_warning=1
echo "UCX_LOG_LEVEL=$UCX_LOG_LEVEL"
echo "SHMEM_DEBUG=$SHMEM_DEBUG"
$oshcc --version

################################################################
#                          TESTS
################################################################

# # --- Hello world ---
# echo $hline ; echo "  Running Hello World test" ; echo $hline
# $oshcc hello_world.c -o ./bin/hello_world
# $oshrun $flags -np 8 ./bin/hello_world
# echo

# # --- shmem_init_thread ---
# echo $hline ; echo "  Running shmem_init_thread test" ; echo $hline
# $oshcc test_init_thread.c -o ./bin/test_init_thread
# $oshrun $flags -np 4 ./bin/test_init_thread
# echo

# # --- shmem_put ---
# echo $hline ; echo "  Running shmem_put test" ; echo $hline
# $oshcc test_put.c -o ./bin/test_put
# $oshrun $flags -np 2 ./bin/test_put
# echo

# # --- shmem_info ---
# echo $hline ; echo "  Running shmem_info test" ; echo $hline
# $oshcc test_info.c -o ./bin/test_info
# $oshrun $flags -np 2 ./bin/test_info
# echo

# # --- shmem_team_create_ctx ---
# echo $hline ; echo "  Running shmem_team_create_ctx test" ; echo $hline
# $oshcc test_team_create_ctx.c -o ./bin/test_team_create_ctx
# $oshrun $flags -np 2 ./bin/test_team_create_ctx
# echo

# # --- shmem_ctx_get_team ---
# echo $hline ; echo "  Running shmem_ctx_get_team test" ; echo $hline
# $oshcc test_ctx_get_team.c -o ./bin/test_ctx_get_team
# # $oshrun $flags -np 2 ./bin/test_ctx_get_team
# $oshrun $flags -np 2 \
#          -x UCX_TLS \
#          -x UCX_RC_DEVX_ENABLED \
#          -x UCX_LOG_LEVEL \
#          -x SHMEM_DEBUG \
#          ./bin/test_ctx_get_team
# echo

# # --- shmem_team_sync ---
# echo $hline ; echo "  Running shmem_team_sync test" ; echo $hline
# $oshcc test_team_sync.c -o ./bin/test_team_sync
# $oshrun $flags -np 2 ./bin/test_team_sync
# echo
# 
# # --- shmem_alltoall ---
# echo $hline ; echo "  Running shmem_alltoall test" ; echo $hline
# $oshcc test_alltoall.c -o ./bin/test_alltoall
# $oshrun $flags -np 4 ./bin/test_alltoall
# echo

# # --- shmem_alltoalls ---
# echo $hline ; echo "  Running shmem_alltoalls test" ; echo $hline
# $oshcc test_alltoalls.c -o ./bin/test_alltoalls
# $oshrun $flags -np 4 ./bin/test_alltoalls
# echo

# # --- shmem_collect ---
# echo $hline ; echo "  Running shmem_collect test" ; echo $hline
# $oshcc test_collect.c -o ./bin/test_collect
# $oshrun $flags -np 4 ./bin/test_collect
# echo

# # --- shmem_fcollect ---
# echo $hline ; echo "  Running shmem_fcollect test" ; echo $hline
# $oshcc test_fcollect.c -o ./bin/test_fcollect
# $oshrun $flags -np 4 ./bin/test_fcollect
# echo

# # --- shmem_broadcast ---
# echo $hline ; echo "  Running shmem_broadcast test" ; echo $hline
# $oshcc test_broadcast.c -o ./bin/test_broadcast
# $oshrun $flags -np 4 ./bin/test_broadcast
# echo

# # --- shmem_reduce ---
# echo $hline ; echo "  Running shmem_reduce test" ; echo $hline
# $oshcc test_reduce.c -o ./bin/test_reduce
# $oshrun $flags -np 4 ./bin/test_reduce
# echo

# # --- shmem_sync ---
# echo $hline ; echo "  Running shmem_sync test" ; echo $hline
# $oshcc test_sync.c -o ./bin/test_sync
# $oshrun $flags -np 4 ./bin/test_sync
# echo

# # --- shmem_alltoall sanity check test ---
# echo $hline ; echo "  Running shmem_alltoall sanity check test" ; echo $hline
# $oshcc test_broken_alltoall.c -o ./bin/test_broken_alltoall
# $oshrun $flags -np 4 ./bin/test_broken_alltoall
# echo

# # --- cannon's algorithm ---
# echo $hline ; echo "  Running cannon's algorithm test" ; echo $hline
# $oshcc canon_algorithm.c -o ./bin/canon_algorithm
# $oshrun $flags -np 4 ./bin/canon_algorithm
# echo

# # --- bfs ---
# echo $hline ; echo "  Running bfs test" ; echo $hline
# $oshcc bfs.c -o ./bin/bfs
# $oshrun $flags -np 4 ./bin/bfs
# echo

# # --- simple ---
# echo $hline ; echo "  Running simple test" ; echo $hline
# $oshcc simple.c -o ./bin/simple
# $oshrun $flags -np 4 ./bin/simple
# echo

# # -- to_all ---
# echo $hline ; echo "  Running to_all test" ; echo $hline
# $oshcc test_to_all.c -o ./bin/test_to_all -Wno-deprecated-declarations
# $oshrun $flags -np 4 ./bin/test_to_all
# echo

# --- shmem_team_my_pe ---
echo $hline ; echo "  Running shmem_team_my_pe test" ; echo $hline
$oshcc test_team_my_pe.c -o ./bin/test_team_my_pe
$oshrun $flags -np 8 ./bin/test_team_my_pe
echo
