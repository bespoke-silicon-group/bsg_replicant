#ifndef BSG_MANYCORE_RESPONDER_H
#define BSG_MANYCORE_RESPONDER_H

#include <bsg_manycore_features.h>
#include <bsg_manycore_packet.h>
#include <bsg_manycore_request_packet_id.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore.h>

#ifdef __cplusplus
extern "C" {
#endif

struct hb_mc_responder;
typedef struct hb_mc_responder hb_mc_responder_t;

typedef int (*hb_mc_responder_init_func)(hb_mc_responder_t *responder,
                                         hb_mc_manycore_t  *mc);

typedef int (*hb_mc_responder_quit_func)(hb_mc_responder_t *responder,
                                         hb_mc_manycore_t *mc);

typedef int (*hb_mc_responder_respond_func)(hb_mc_responder_t *responder,
                                            hb_mc_manycore_t *mc,
                                            const hb_mc_request_packet_t *rqst);

struct hb_mc_responder {
        const char *name;
        void *responder_data;
        hb_mc_request_packet_id_t *ids;

        __attribute__((warn_unused_result))
        hb_mc_responder_init_func    init;

        __attribute__((warn_unused_result))
        hb_mc_responder_quit_func    quit;

        __attribute__((warn_unused_result))
        hb_mc_responder_respond_func respond;
#ifdef __cplusplus
        hb_mc_responder(const char *name,
                        hb_mc_request_packet_id_t *ids = nullptr,
                        hb_mc_responder_init_func init = nullptr,
                        hb_mc_responder_quit_func quit = nullptr,
                        hb_mc_responder_respond_func respond = nullptr):
        name(name), ids(ids), init(init), quit(quit), respond(respond) {}
#endif
};

/**
 * Initialze all registered responders.
 * This function is generally called from within the manycore init interface.
 * @param[in] mc  A manycore.
 * @return HB_MC_SUCCESS if succesful. An error code otherwise.
 */
__attribute__((warn_unused_result))
int hb_mc_responders_init(hb_mc_manycore_t *mc);

/**
 * Initialize a single responder.
 * This function is generally called by hb_mc_responders_init() but should also be
 * called by client code after it adds a responder with hb_mc_responder_add().
 * @param[in] responder A responder to initialize.
 * @param[in] mc        A manycore initialized with hb_mc_manycore_init().
 * @return HB_MC_SUCCESS if succesful. An error code otherwise.
 */
__attribute__((warn_unused_result))
int hb_mc_responder_init(hb_mc_responder_t *responder, hb_mc_manycore_t *mc);

/**
 * Cleanup a single responder.
 * This function is generally called by hb_mc_responders_quit() but should also be
 * called by client code before it removes a responder with hb_mc_responder_del().
 * @param[in] responder  A responder to cleanup.
 * @Param[in] mc         A manycore initialized with hb_mc_manycore_init().
 * @return HB_MC_SUCCESS if succesful. An error code otherwise.
 */
__attribute__((warn_unused_result))
int hb_mc_responder_quit(hb_mc_responder_t *responder, hb_mc_manycore_t *mc);

/**
 * Cleanup all responders.
 * This function is generally called by hb_mc_manycore_exit().
 * @param[in] mc  A manycore.
 * @return HB_MC_SUCCESS if succesful. An error code otherwise.
 */
__attribute__((warn_unused_result))
int hb_mc_responders_quit(hb_mc_manycore_t *mc);

/**
 * Let all responders respond to a manycore request packet.
 * @param[in] mc       A manycore.
 * @param[in] request  A request packet.
 * @return HB_MC_SUCCESS if succesful. An error code otherwise.
 */
__attribute__((warn_unused_result))
int hb_mc_responders_respond(hb_mc_manycore_t *mc, const hb_mc_request_packet_t *request);

/**
 * Add a responder to the global list of responders.
 * @param[in] responder  A new responder. This should ** NOT ** be initialized with hb_mc_responder_init().
 * @return HB_MC_SUCCESS if succesful. An error code otherwise.
 */
__attribute__((warn_unused_result))
int hb_mc_responder_add(hb_mc_responder_t *responder);

/**
 * Remove a responder from the global list of responders.
 * @param[in] responder  A responder to remove. This ** MUST ** have been cleanuped up with hb_mc_responder_quit().
 * @return HB_MC_SUCCESS if succesful. An error code otherwise.
 */
__attribute__((warn_unused_result))
int hb_mc_responder_del(hb_mc_responder_t *responder);

#ifdef __cplusplus
#define source_responder(rspdr)						\
	namespace {							\
		class __bsg_manycore_ ## rspdr ## _add_del {		\
		public:							\
			__bsg_manycore_ ## rspdr ## _add_del () {	\
				int r = hb_mc_responder_add(&rspdr);	\
				if (r != HB_MC_SUCCESS)			\
					throw r;			\
			}						\
			~__bsg_manycore_ ## rspdr ## _add_del () {	\
				int r = hb_mc_responder_del(&rspdr);	\
				if (r != HB_MC_SUCCESS)			\
					throw r;			\
			}						\
		};							\
		__bsg_manycore_ ## rspdr ## _add_del add_ ## rspdr;	\
	}
#else
#define source_responder(rspdr)                                         \
        __attribute__((constructor(65535), visibility("internal")))	\
        void __bsg_manycore_ ## rspdr ## _add(void) {			\
                int r = hb_mc_responder_add(&rspdr);                    \
                if (r != HB_MC_SUCCESS)                                 \
                        bsg_pr_err("%s: failed to add responder %s: %s\n", \
                                   __FILE__, #rspdr, hb_mc_strerror(r)); \
        }                                                               \
        __attribute__((destructor(65535), visibility("internal")))	\
        void __bsg_manycore_ ## rspdr ## _del(void) {			\
                int r = hb_mc_responder_del(&rspdr);                    \
                if (r != HB_MC_SUCCESS)                                 \
                        bsg_pr_err("%s: failed to remove responder %s: %s\n", \
                                   __FILE__, #rspdr, hb_mc_strerror(r)); \
        }
#endif

#ifdef __cplusplus
}
#endif

#endif
