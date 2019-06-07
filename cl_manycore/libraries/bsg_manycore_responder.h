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

__attribute__((warn_unused_result))
int hb_mc_responders_init(hb_mc_manycore_t *mc);

__attribute__((warn_unused_result))
int hb_mc_responders_quit(hb_mc_manycore_t *mc);

__attribute__((warn_unused_result))
int hb_mc_responders_respond(hb_mc_manycore_t *mc);

__attribute__((warn_unused_result))
int hb_mc_responder_add(hb_mc_responder_t *responder);

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
