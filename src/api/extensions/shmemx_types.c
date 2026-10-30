#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "shmemx_types.h"
#include "shmemu.h"
#include "hashmap.h"

struct hashmap *type_map = NULL;

int compare_contents (shmemx_datatype_contents *ca, shmemx_datatype_contents *cb){
    if (ca == NULL || cb == NULL) return 1;
    if (ca == cb) return 0;
    return (ca->combiner == cb->combiner && ca->nr_ints == cb->nr_ints && ca->nr_aints == cb->nr_aints && ca->nr_types == cb->nr_types);
}

int dtype_compare (const void *a, vonst void *b, void *data){
    NO_WARN_UNUSED (data);

    if (a==b) return 0;
    const shmemx_typed_entry_t *ua = (const shmemx_typed_entry_t *) a;
    const shmemx_typed_entry_t *ub = (const shmemx_typed_entry_t *) b;
    shmem_internal_datatype_t *ta = ua->representation;
    shmem_internal_datatype_t *tb = ub->representation;

    if (ta->handle == tb->handle) return 0;

    if (ta-> size == tb->size && ta->extent == tb->extent && tb->ub == ta->ub && ta->lb == tb->lb && ta->true_ub == tb->true_ub && ta->true_lb == tb->true_lb && ta->basic_type == tb->basic_type && compare_contents(ta->contents, tb->contents) == 0){
        return 0;
    }
    return 1;
}

bool dtype_iter (const void *item, void *tdata){
    NO_WARN_UNUSED(tdata);
    const shmemx_typed_entry_t *a = (const shmemx_typed_entry_t *)item;
    printf("Type handle: %d, pointer internal: %p\n", a->handle, a->representation);
    return true;
}


uint64_t dtype_hash (const void *item, uint64_t seed0, uint64_t seed1){
    const shmem_typed_entry_t *entry = (const shmem_typed_entry_t *)item;
    return hashmap_sip(entry->handle, sizeof(shmemx_datatype_handle), seed0, seed1);
}

/* Nathan Reed's 64-bit random hash */
uint64_t squirrel3_hash(uint64_t position, uint64_t seed) {
    const uint64_t BIT_NOISE1 = 0xB5297A4DB5297A4D;
    const uint64_t BIT_NOISE2 = 0x68E31DA468E31DA4;
    const uint64_t BIT_NOISE3 = 0x1B56C4E91B56C4E9;

    uint64_t mangled = position + seed;
    mangled *= BIT_NOISE1;
    mangled ^= (mangled >> 8);
    mangled += BIT_NOISE2;
    mangled ^= (mangled << 8);
    mangled *= BIT_NOISE3;
    mangled ^= (mangled >> 8);
    return mangled;
}

int shmemx_dtype_init(){
    type_map = hashmap_new(sizeof(shmemx_typed_entry_t), 0,
            squirrel3_hash(0, 0x32324545), squirrel3_hash(8, 0x45456767), NULL, NULL);

    if (type_map == NULL){
        ERROR_SHMEM("derived datatype hashmap failed to be created\n");
        return ENOMEM;
    }

    return 0;
}

void shmemx_dtype_finalize(){
    hashmap_free(type_map);
    type_map = NULL;
    return;
}

shmemx_internal_datatype_t *handle_allocation(){

    shmemx_internal_datatype_t *new_rep = (shmemx_internal_datatype_t *)calloc(1, sizeof(shmemx_internal_type_t));
    if (new_rep == NULL){
        ERROR_SHMEM("Failed to allocate space for a new type representation\n");
        return NULL;
    }
    return new_rep;    
}



int create_zerolen_type(shmemx_datatype *new_type){
    int shmemx_success = 0;
    int is_builtin = 0;
    shmemx_internal_int elem_size = 0;
    shmemx_datatype el_type = 0;
    shmemx_internal_datatype_t *new_type = NULL;

    

    return -1;
}



int shmemx_dtype_create_contig(int elem_count, shmemx_datatype old_type, shmemx_datatype *new_type){
    int shmemx_success = 0;
    int is_builtin = 0;
    shmemx_datatype el_type;
    shmemx_internal_datatype_t *new_internal_type = NULL;
    if (elem_count == 0){

        return create_zerolen_type(new_type);
    }


fn_fail:
    ERROR_SHMEM("Unable to create datatype for reason %d\n", shmemx_success);
    
fn_exit:
    return shmemx_success;
}
