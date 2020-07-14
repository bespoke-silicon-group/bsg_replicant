#ifndef __BSG_MANYCORE_TRACER_HPP
#define __BSG_MANYCORE_TRACER_HPP
#include <bsg_manycore.h>
#include <string>

// Since the definition of hb_mc_tracer_t is implementation
// dependent, we use void *
typedef void* hb_mc_tracer_t;

#ifdef __cplusplus
extern "C" {
#endif

        /**
         * Initialize an hb_mc_tracer_t instance
         * @param[in] p    A pointer to the hb_mc_tracer_t instance to initialize
         * @param[in] hier An implementation-dependent string. See the implementation for more details.
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_tracer_init(hb_mc_tracer_t *p, std::string &hier);

        /**
         * Clean up an hb_mc_tracer_t instance
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_tracer_cleanup(hb_mc_tracer_t *p);

        /**
         * Enable trace file generation (vanilla_operation_trace.csv)
         * @param[in] p    A tracer instance initialized with hb_mc_tracer_init()
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_tracer_trace_enable(hb_mc_tracer_t p);

        /**
         * Disable trace file generation (vanilla_operation_trace.csv)
         * @param[in] p    A tracer instance initialized with hb_mc_tracer_init()
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_tracer_trace_disable(hb_mc_tracer_t p);

        /**
         * Enable log file generation (vanilla.log)
         * @param[in] p    A tracer instance initialized with hb_mc_tracer_init()
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_tracer_log_enable(hb_mc_tracer_t p);

        /**
         * Disable log file generation (vanilla.log)
         * @param[in] p    A tracer instance initialized with hb_mc_tracer_init()
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_tracer_log_disable(hb_mc_tracer_t p);

#ifdef __cplusplus
}
#endif

#endif
