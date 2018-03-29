/* For license: see LICENSE file at top-level */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"
#include "state.h"

#include "shmem/defs.h"

#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <ucp/api/ucp.h>

/*
 * -- helpers ----------------------------------------------------------------
 */

/*
 * shortcut to look up the UCP endpoint of a context
 */
inline static ucp_ep_h
lookup_ucp_ep(shmemc_context_h ch /* unused */, int pe)
{
    return proc.comms.eps[pe];
}

/*
 * find remote rkey
 */
inline static ucp_rkey_h
lookup_rkey(size_t region, int pe)
{
    return proc.comms.regions[region].minfo[pe].racc.rkey;
}

/*
 * where the heap lives
 */
inline static uint64_t
get_base(size_t region, int pe)
{
    return proc.comms.regions[region].minfo[pe].base;
}

/*
 * -- translation helpers ---------------------------------------------------
 */

inline static int
in_region(uint64_t addr, size_t region, int pe)
{
    const mem_info_t mi = proc.comms.regions[region].minfo[pe];

    return (mi.base <= addr) && (addr < mi.end);
}

/*
 * find memory region that ADDR is in, or -1 if none
 */
inline static long
lookup_region(uint64_t addr, int pe)
{
    long r;

    /*
     * Let's search down from top heap to globals (#0) under
     * assumption most data in heaps and newest one is most likely
     * (may need to revisit)
     */
    for (r = proc.comms.nregions - 1; r >= 0; r -= 1) {
        if (in_region(addr, (size_t) r, pe)) {
            return r;
            /* NOT REACHED */
        }
    }

    return -1L;
}

/*
 * translate remote address:
 *
 * if all addresses aligned, remote always == local
 *
 * otherwise globals are always aligned, but translate shmalloc'ed
 * variables
 */
inline static uint64_t
translate_address(uint64_t local_addr, size_t region, int pe)
{
#ifdef ENABLE_ALIGNED_ADDRESSES
    return local_addr;
#else
    if (region == 0) {
        return local_addr;
    }
    else {
        const uint64_t my_offset = local_addr - get_base(region, proc.rank);
        const uint64_t remote_addr = my_offset + get_base(region, pe);

        return remote_addr;
    }
#endif /* ENABLE_ALIGNED_ADDRESSES */
}

/*
 * All ops here need to find remote keys and addresses
 */
inline static void
get_remote_key_and_addr(uint64_t local_addr, int pe,
                        ucp_rkey_h *rkey_p, uint64_t *raddr_p)
{
    const long r = lookup_region(local_addr, proc.rank);

    shmemu_assert("remote key/address lookup", r >= 0);

    *rkey_p = lookup_rkey(r, pe);
    *raddr_p = translate_address(local_addr, r, pe);
}

/*
 *  -- helpers for atomics -----------------------------------------------
 */

/*
 * a dummy callback that does nothing
 */

static void
noop_callback(void *request, ucs_status_t status)
{
    logger(LOG_INFO,
           "AMO didn't complete immediately, using callback");
}

/*
 * wait for some non-blocking request to complete on a worker
 *
 *
 * TODO: possible consolidation with EP disconnect code
 *
 */

inline static ucs_status_t
wait_for_request(void *req, ucp_worker_h w)
{
    if (req == NULL) {          /* completed */
        return UCS_OK;
    }
    else if (UCS_PTR_IS_ERR(req)) {
        return UCS_PTR_STATUS(req);
    }
    else {                      /* wait for completion */
        ucs_status_t s;

        do {
            (void) ucp_worker_progress(w);
#ifdef HAVE_UCP_REQUEST_CHECK_STATUS
            s = ucp_request_check_status(req);
#else
            s = ucp_request_test(req, NULL);
#endif  /* HAVE_UCP_REQUEST_CHECK_STATUS */
        } while (s == UCS_INPROGRESS);

        ucp_request_free(req);
        return s;
    }
}

/*
 * postor-fetch--and-wait AMO to target address "t" on PE "pe" with
 * value "v"
 */

inline static ucs_status_t
helper_atomic_post_op(ucp_atomic_post_op_t uapo,
                      shmemc_context_h ch,
                      uint64_t t,
                      uint64_t v,    /* encapsulate 32/64-bit value */
                      size_t vs,     /* actual size of value */
                      int pe)
{
    uint64_t r_t;
    ucp_rkey_h rkey;
    ucp_ep_h ep;

    get_remote_key_and_addr(t, pe, &rkey, &r_t);
    ep = lookup_ucp_ep(ch, pe);

    return ucp_atomic_post(ep, uapo, v, vs, r_t, rkey);
}

inline static ucs_status_t
helper_atomic_fetch_op(ucp_atomic_fetch_op_t uafo,
                       shmemc_context_h ch,
                       uint64_t t,
                       uint64_t v,    /* encapsulate 32/64-bit value */
                       size_t vs,     /* actual size of value */
                       int pe,
                       uint64_t *result)
{
    uint64_t r_t;
    ucp_rkey_h rkey;
    ucp_ep_h ep;
    ucs_status_ptr_t sp;

    get_remote_key_and_addr(t, pe, &rkey, &r_t);
    ep = lookup_ucp_ep(ch, pe);

    sp = ucp_atomic_fetch_nb(ep, uafo, v, result, vs, r_t, rkey,
                             noop_callback);
    if (sp == UCS_OK) {
        return UCS_OK;          /* completed */
    }
    else if (UCS_PTR_IS_ERR(sp)) { /* error */
        return UCS_PTR_STATUS(sp);
    }
    else {                      /* wait */
        return wait_for_request(sp, ch->w);
    }
}

/* TODO: repeated patterns here, maybe some kind of template? */

/*
 * adds
 */

#define HELPER_FADD(_size)                                          \
    inline static uint##_size##_t                                   \
    helper_atomic_fetch_add##_size(shmemc_context_h ch,             \
                                   uint64_t t, uint##_size##_t v,   \
                                   int pe)                          \
    {                                                               \
        uint64_t ret = 0;                                           \
        ucs_status_t s;                                             \
                                                                    \
        s = helper_atomic_fetch_op(UCP_ATOMIC_FETCH_OP_FADD,        \
                                   ch,                              \
                                   t,                               \
                                   v, sizeof(v),                    \
                                   pe,                              \
                                   &ret);                           \
        assert(s == UCS_OK);                                        \
                                                                    \
        return (uint##_size##_t) ret;                               \
    }

HELPER_FADD(32)
HELPER_FADD(64)

#define HELPER_ADD(_size)                                   \
    inline static void                                      \
    helper_atomic_add##_size(shmemc_context_h ch,           \
                             uint64_t t, uint##_size##_t v, \
                             int pe)                        \
    {                                                       \
        ucs_status_t s;                                     \
                                                            \
        s = helper_atomic_post_op(UCP_ATOMIC_POST_OP_ADD,   \
                                  ch,                       \
                                  t,                        \
                                  v, sizeof(v),             \
                                  pe);                      \
        assert(s == UCS_OK);                                \
    }

HELPER_ADD(32)
HELPER_ADD(64)

/*
 * increments use add
 */

#define HELPER_FINC(_size)                                      \
    inline static uint##_size##_t                               \
    helper_atomic_fetch_inc##_size(shmemc_context_h ch,         \
                                   uint64_t t, int pe)          \
    {                                                           \
        return helper_atomic_fetch_add##_size(ch, t, 1, pe);    \
    }

HELPER_FINC(32)
HELPER_FINC(64)

#define HELPER_INC(_size)                                   \
    inline static void                                      \
    helper_atomic_inc##_size(shmemc_context_h ch,           \
                             uint64_t t, int pe)            \
    {                                                       \
        (void) helper_atomic_fetch_inc##_size(ch, t, pe);   \
    }

HELPER_INC(32)
HELPER_INC(64)

/*
 * swaps
 */

#define HELPER_SWAP(_size)                                      \
    inline static uint##_size##_t                               \
    helper_atomic_swap##_size(shmemc_context_h ch,              \
                              uint64_t t, uint##_size##_t v,    \
                              int pe)                           \
    {                                                           \
        uint64_t r_t;                                           \
        ucp_rkey_h rkey;                                        \
        uint##_size##_t ret;                                    \
        ucp_ep_h ep;                                            \
        ucs_status_t s;                                         \
                                                                \
        get_remote_key_and_addr(t, pe, &rkey, &r_t);            \
        ep = lookup_ucp_ep(ch, pe);                             \
                                                                \
        s = ucp_atomic_swap##_size(ep, v, r_t, rkey, &ret);     \
        assert(s == UCS_OK);                                    \
                                                                \
        return ret;                                             \
    }

HELPER_SWAP(32)
HELPER_SWAP(64)

#define HELPER_CSWAP(_size)                                             \
    inline static uint##_size##_t                                       \
    helper_atomic_cswap##_size(shmemc_context_h ch,                     \
                               uint64_t t,                              \
                               uint##_size##_t c, uint##_size##_t v,    \
                               int pe)                                  \
    {                                                                   \
        uint64_t r_t;                                                   \
        ucp_rkey_h rkey;                                                \
        uint##_size##_t ret;                                            \
        ucp_ep_h ep;                                                    \
        ucs_status_t s;                                                 \
                                                                        \
        get_remote_key_and_addr(t, pe, &rkey, &r_t);                    \
        ep = lookup_ucp_ep(ch, pe);                                     \
                                                                        \
        s = ucp_atomic_cswap##_size(ep, c, v, r_t, rkey, &ret);         \
        assert(s == UCS_OK);                                            \
                                                                        \
        return ret;                                                     \
    }

HELPER_CSWAP(32)
HELPER_CSWAP(64)

/*
 * bitwise helpers
 */

#ifndef HAVE_UCP_BITWISE_ATOMICS

/* NB UCX currently doesn't have API support for these ops */

#define NOTUCP_ATOMIC_BITWISE_OP(_op, _opname, _size)               \
    inline static ucs_status_t                                      \
    ucp_atomic_##_opname##_size(ucp_ep_h ep,                        \
                                uint##_size##_t val,                \
                                uint64_t remote_addr,               \
                                ucp_rkey_h rkey,                    \
                                uint##_size##_t *result)            \
    {                                                               \
        uint##_size##_t rval, rval_orig, ret;                       \
        ucs_status_t s;                                             \
                                                                    \
        do {                                                        \
            s = ucp_get(ep, &rval_orig, sizeof(rval_orig),          \
                        remote_addr, rkey);                         \
            assert(s == UCS_OK);                                    \
                                                                    \
            rval = (rval_orig) _op (val);                           \
                                                                    \
            s = ucp_atomic_cswap##_size(ep, rval_orig, rval,        \
                                        remote_addr, rkey, &ret);   \
            assert(s == UCS_OK);                                    \
        } while (ret != rval_orig);                                 \
                                                                    \
        *result = ret;                                              \
        return UCS_OK;                                              \
    }

NOTUCP_ATOMIC_BITWISE_OP(&, and, 32)
NOTUCP_ATOMIC_BITWISE_OP(&, and, 64)
NOTUCP_ATOMIC_BITWISE_OP(|, or, 32)
NOTUCP_ATOMIC_BITWISE_OP(|, or, 64)
NOTUCP_ATOMIC_BITWISE_OP(^, xor, 32)
NOTUCP_ATOMIC_BITWISE_OP(^, xor, 64)

#endif  /* ! HAVE_UCP_BITWISE_ATOMICS */

#define HELPER_FETCH_BITWISE_OP(_op, _opname, _size)                \
    inline static uint##_size##_t                                   \
    helper_atomic_fetch_##_opname##_size(shmemc_context_h ch,       \
                                         uint64_t t,                \
                                         uint##_size##_t v,         \
                                         int pe)                    \
    {                                                               \
        uint64_t r_t;                                               \
        uint##_size##_t ret;                                        \
        ucp_rkey_h rkey;                                            \
        ucp_ep_h ep;                                                \
        ucs_status_t s;                                             \
                                                                    \
        get_remote_key_and_addr(t, pe, &rkey, &r_t);                \
        ep = lookup_ucp_ep(ch, pe);                                 \
                                                                    \
        s = ucp_atomic_##_opname##_size(ep, v, r_t, rkey, &ret);    \
        assert(s == UCS_OK);                                        \
                                                                    \
        return ret;                                                 \
    }

HELPER_FETCH_BITWISE_OP(&, and, 32)
HELPER_FETCH_BITWISE_OP(&, and, 64)
HELPER_FETCH_BITWISE_OP(|, or, 32)
HELPER_FETCH_BITWISE_OP(|, or, 64)
HELPER_FETCH_BITWISE_OP(^, xor, 32)
HELPER_FETCH_BITWISE_OP(^, xor, 64)

/**
 * API
 *
 **/

/*
 * -- ordering -----------------------------------------------------------
 */

void
shmemc_ctx_fence(shmem_ctx_t ctx)
{
    shmemc_context_h ch = (shmemc_context_h) ctx;

    if (! ch->attr.nostore) {
        const ucs_status_t s = ucp_worker_fence(ch->w);

        assert(s == UCS_OK);
    }
}

void
shmemc_ctx_quiet(shmem_ctx_t ctx)
{
    shmemc_context_h ch = (shmemc_context_h) ctx;

    if (! ch->attr.nostore) {
        const ucs_status_t s = ucp_worker_flush(ch->w);

        assert(s == UCS_OK);
    }
}

/*
 * This should be correct, but not optimal.  Gets us going.
 */

#ifdef ENABLE_EXPERIMENTAL

int
shmemc_ctx_fence_test(shmem_ctx_t ctx)
{
    shmemc_ctx_fence(ctx);
    return 1;
}

int
shmemc_ctx_quiet_test(shmem_ctx_t ctx)
{
    shmemc_ctx_quiet(ctx);
    return 1;
}

#endif  /* ENABLE_EXPERIMENTAL */

/*
 * -- accessible memory pointers -----------------------------------------
 */

/*
 * See if addr is reachable using given context.  Return usable
 * address if so, otherwise NULL.
 */

void *
shmemc_ctx_ptr(shmem_ctx_t ctx, const void *addr, int pe)
{
    /* check to see if UCX is new enough */
#ifdef HAVE_UCP_RKEY_PTR
    uint64_t r_addr;            /* address on other PE */
    ucp_rkey_h rkey;            /* rkey for remote address */
    void *usable_addr = NULL;
    ucs_status_t s;

    get_remote_key_and_addr((uint64_t) addr, pe, &rkey, &r_addr);

    s = ucp_rkey_ptr(rkey, r_addr, &usable_addr);
    if (s == UCS_OK) {
        return usable_addr;
        /* NOT REACHED */
    }
    /*
     * fall through and ...
     */
#endif  /* HAVE_UCP_RKEY_PTR */

    return NULL;
}

/*
 * Return non-zero if adddress is remotely accessible, 0 otherwise
 */
int
shmemc_addr_accessible(const void *addr, int pe)
{
    return lookup_region((uint64_t) addr, pe) >= 0;
}

/*
 * Return non-zero if a valid PE #, 0 otherwise
 */
int
shmemc_pe_accessible(int pe)
{
    return IS_VALID_PE_NUMBER(pe);
}

/*
 * -- puts & gets --------------------------------------------------------
 */

void
shmemc_ctx_put(shmem_ctx_t ctx,
               void *dest, const void *src,
               size_t nbytes, int pe)
{
    uint64_t rdest;             /* address on other PE */
    ucp_rkey_h rkey;            /* rkey for remote address */
    ucp_ep_h ep;
    ucs_status_t s;

    get_remote_key_and_addr((uint64_t) dest, pe, &rkey, &rdest);
    ep = lookup_ucp_ep(ctx, pe);

    s = ucp_put(ep, src, nbytes, rdest, rkey);
    assert(s == UCS_OK);
}

void
shmemc_ctx_get(shmem_ctx_t ctx,
               void *dest, const void *src,
               size_t nbytes, int pe)
{
    uint64_t r_src;
    ucp_rkey_h rkey;
    ucp_ep_h ep;
    ucs_status_t s;

    get_remote_key_and_addr((uint64_t) src, pe, &rkey, &r_src);
    ep = lookup_ucp_ep(ctx, pe);

    s = ucp_get(ep, dest, nbytes, r_src, rkey);
    assert(s == UCS_OK);
}

/*
 * strided ops currently build on put/get in upper API
 */

/**
 * Return status from UCP nbi routines probably needs more handling
 *
 */

void
shmemc_ctx_put_nbi(shmem_ctx_t ctx,
                   void *dest, const void *src,
                   size_t nbytes, int pe)
{
    uint64_t rdest;
    ucp_rkey_h rkey;
    ucp_ep_h ep;
    ucs_status_t s;

    get_remote_key_and_addr((uint64_t) dest, pe, &rkey, &rdest);
    ep = lookup_ucp_ep(ctx, pe);

    s = ucp_put_nbi(ep, src, nbytes, rdest, rkey);
    assert(s == UCS_OK || s == UCS_INPROGRESS);
}

void
shmemc_ctx_get_nbi(shmem_ctx_t ctx,
                   void *dest, const void *src,
                   size_t nbytes, int pe)
{
    uint64_t r_src;
    ucp_rkey_h rkey;
    ucp_ep_h ep;
    ucs_status_t s;

    get_remote_key_and_addr((uint64_t) src, pe, &rkey, &r_src);
    ep = lookup_ucp_ep(ctx, pe);

    s = ucp_get_nbi(ep, dest, nbytes, r_src, rkey);
    assert(s == UCS_OK || s == UCS_INPROGRESS);
}

/*
 * -- atomics ------------------------------------------------------------
 */

/**
 * AMO API
 **/

/*
 * add
 */

#define SHMEMC_CTX_ADD(_size)                               \
    void                                                    \
    shmemc_ctx_add##_size(shmem_ctx_t ctx,                  \
                          void *t, uint64_t v, int pe)      \
    {                                                       \
        helper_atomic_add##_size(ctx, (uint64_t) t, v, pe); \
    }

SHMEMC_CTX_ADD(32)
SHMEMC_CTX_ADD(64)

/*
 * inc
 */

#define SHMEMC_CTX_INC(_size)                               \
    void                                                    \
    shmemc_ctx_inc##_size(shmem_ctx_t ctx,                  \
                          void *t, int pe)                  \
    {                                                       \
        helper_atomic_inc##_size(ctx, (uint64_t) t, pe);    \
    }

SHMEMC_CTX_INC(32)
SHMEMC_CTX_INC(64)

/*
 * fetch-and-add
 */

#define SHMEMC_CTX_FADD(_size)                                      \
    uint64_t                                                        \
    shmemc_ctx_fadd##_size(shmem_ctx_t ctx,                         \
                           void *t, uint64_t v, int pe)             \
    {                                                               \
        return helper_atomic_fetch_add##_size(ctx, (uint64_t) t,    \
                                              v, pe);               \
    }

SHMEMC_CTX_FADD(32)
SHMEMC_CTX_FADD(64)

/*
 * fetch-and-inc
 */

#define SHMEMC_CTX_FINC(_size)                                      \
    uint64_t                                                        \
    shmemc_ctx_finc##_size(shmem_ctx_t ctx,                         \
                           void *t, int pe)                         \
    {                                                               \
        return helper_atomic_fetch_inc##_size(ctx, (uint64_t) t,    \
                                              pe);                  \
    }

SHMEMC_CTX_FINC(32)
SHMEMC_CTX_FINC(64)

/*
 * swaps
 */

#define SHMEMC_CTX_SWAP(_size)                                          \
    uint64_t                                                            \
    shmemc_ctx_swap##_size(shmem_ctx_t ctx,                             \
                           void *t, uint64_t v, int pe)                 \
    {                                                                   \
        return helper_atomic_swap##_size(ctx, (uint64_t) t, v, pe);     \
    }

SHMEMC_CTX_SWAP(32)
SHMEMC_CTX_SWAP(64)

#define SHMEMC_CTX_CSWAP(_size)                                         \
    uint64_t                                                            \
    shmemc_ctx_cswap##_size(shmem_ctx_t ctx,                            \
                            void *t, uint64_t c, uint64_t v, int pe)    \
    {                                                                   \
        return helper_atomic_cswap##_size(ctx, (uint64_t) t, c, v, pe); \
    }

SHMEMC_CTX_CSWAP(32)
SHMEMC_CTX_CSWAP(64)

/*
 * fetch & set
 *
 * TODO: UCX really does have a fetch_nb, but for now do something
 * simple
 *
 */

#define SHMEMC_CTX_FETCH(_size)                                     \
    uint64_t                                                        \
    shmemc_ctx_fetch##_size(shmem_ctx_t ctx,                        \
                            void *t, int pe)                        \
    {                                                               \
        return helper_atomic_fetch_add##_size(ctx, (uint64_t) t,    \
                                              0, pe);               \
    }

SHMEMC_CTX_FETCH(32)
SHMEMC_CTX_FETCH(64)

/*
 * TODO: 3/27/18:
 *
 * set/fetch will likely turn into UCP no-op post operations
 */
#define SHMEMC_CTX_SET(_size)                                       \
    void                                                            \
    shmemc_ctx_set##_size(shmem_ctx_t ctx,                          \
                          void *t, uint64_t v, int pe)              \
    {                                                               \
        (void) helper_atomic_swap##_size(ctx, (uint64_t) t, v, pe); \
    }

SHMEMC_CTX_SET(32)
SHMEMC_CTX_SET(64)

/*
 * fetched-bitwise
 */

#define SHMEMC_CTX_FETCH_BITWISE(_op, _size)                        \
    uint64_t                                                        \
    shmemc_ctx_fetch_##_op##_size(shmem_ctx_t ctx,                  \
                                  void *t, uint64_t v, int pe)      \
    {                                                               \
        return helper_atomic_fetch_##_op##_size(ctx, (uint64_t) t,  \
                                                v, pe);             \
    }

SHMEMC_CTX_FETCH_BITWISE(and, 32)
SHMEMC_CTX_FETCH_BITWISE(and, 64)

SHMEMC_CTX_FETCH_BITWISE(or, 32)
SHMEMC_CTX_FETCH_BITWISE(or, 64)

SHMEMC_CTX_FETCH_BITWISE(xor, 32)
SHMEMC_CTX_FETCH_BITWISE(xor, 64)

/*
 * bitwise
 */

/*
 * TODO: 3/27/18:
 *
 * bitwise ops slated to appear soon in revamped UCP API, need to
 * revisit when that happens
 */

#define SHMEMC_CTX_BITWISE(_op, _size)                              \
    void                                                            \
    shmemc_ctx_##_op##_size(shmem_ctx_t ctx,                        \
                            void *t, uint64_t v, int pe)            \
    {                                                               \
        (void) helper_atomic_fetch_##_op##_size(ctx, (uint64_t) t,  \
                                                v, pe);             \
    }

SHMEMC_CTX_BITWISE(and, 32)
SHMEMC_CTX_BITWISE(and, 64)

SHMEMC_CTX_BITWISE(or, 32)
SHMEMC_CTX_BITWISE(or, 64)

SHMEMC_CTX_BITWISE(xor, 32)
SHMEMC_CTX_BITWISE(xor, 64)
