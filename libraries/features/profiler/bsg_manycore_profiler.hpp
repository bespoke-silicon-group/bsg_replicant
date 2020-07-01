#ifndef __BSG_MANYCORE_PROFILER_HPP
#define __BSG_MANYCORE_PROFILER_HPP
#include <bsg_manycore.h>
#include <string>

// Since the definition of hb_mc_profiler_t is implementation
// dependent, we use void *
typedef void* hb_mc_profiler_t;

#ifdef __cplusplus
extern "C" {
#endif

        /**
         * Initialize an hb_mc_profiler_t instance
         * @param[in] p    A pointer to the hb_mc_profiler_t instance to initialize
         * @param[in] x    X dimension of the Manycore Device
         * @param[in] y    Y dimension of the Manycore Device
         * @param[in] hier An implementation-dependent string. See the implementation for more details.
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_profiler_init(hb_mc_profiler_t *p, hb_mc_idx_t x, hb_mc_idx_t y, std::string &hier);

        /**
         * Clean up an hb_mc_profiler_t instance
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_profiler_cleanup(hb_mc_profiler_t *p);

        /**
         * Get the number of instructions executed for a certain class of instructions
         * @param[in] p     A hb_mc_profiler_t instance initialized with hb_mc_profiler_init()
         * @param[in] itype An enum defining the class of instructions to query.
         * @param[out] count The number of instructions executed in the queried class.
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_profiler_get_icount(hb_mc_profiler_t p, bsg_instr_type_e itype, int *count);

#ifdef __cplusplus
}
#endif

#endif
