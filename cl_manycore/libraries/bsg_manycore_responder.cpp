#include <bsg_manycore_responder.h>
#include <bsg_manycore_errno.h>
#include <list>
#include <stdint.h>

typedef std::list<hb_mc_responder_t *> responder_list;

static responder_list *responders = nullptr;

static int hb_mc_responder_init(hb_mc_responder_t *responder, hb_mc_manycore_t *mc)
{
	int err;
	if (responder->init == nullptr)
		return HB_MC_INVALID;

	err = responder->init(responder, mc);
	if (err != HB_MC_SUCCESS)
		return err;

	return HB_MC_SUCCESS;
}

int hb_mc_responders_init(hb_mc_manycore_t *mc)
{
	if (responders == nullptr)
		return HB_MC_FAIL;

	for (auto it = responders->begin();
	     it != responders->end();
	     it++) {
		auto responder = *it;
		int err = hb_mc_responder_init(responder, mc);
		if (err != HB_MC_SUCCESS)
			return err;
	}

	return HB_MC_SUCCESS;
}

static int hb_mc_responder_quit(hb_mc_responder_t *responder, hb_mc_manycore_t *mc)
{
	int err;
	if (responder->quit == nullptr)
		return HB_MC_INVALID;

	err = responder->quit(responder, mc);
	if (err != HB_MC_SUCCESS)
		return err;

	return HB_MC_SUCCESS;
}

int hb_mc_responders_quit(hb_mc_manycore_t *mc)
{
	int err;

	if (responders == nullptr)
		return HB_MC_FAIL;

	for (auto it = responders->begin();
	     it != responders->end();
	     it++) {
		auto responder = *it;
		err = hb_mc_responder_quit(responder, mc);
		if (err != HB_MC_SUCCESS)
			return err;
	}

	return HB_MC_SUCCESS;
}

static int hb_mc_responder_respond(hb_mc_responder_t *responder,
			    hb_mc_manycore_t *mc,
			    const hb_mc_request_packet_t *rqst)
{
	int err;

	if (responder->respond == nullptr)
		return HB_MC_INVALID;

	if (responder->ids == nullptr)
		return HB_MC_INVALID;

	for (hb_mc_request_packet_id_t *id = responder->ids;
	     id->init != 0;
	     id++) {
		if (hb_mc_request_packet_is_match(rqst, id) == 1)
			return responder->respond(responder, mc, rqst);
	}

	return HB_MC_SUCCESS;
}

int hb_mc_responders_respond(hb_mc_manycore_t *mc, const hb_mc_request_packet_t *rqst)
{
	int err;

	if (responders == nullptr)
		return HB_MC_FAIL;

	for (auto it = responders->begin();
	     it != responders->end();
	     it++) {
		auto responder = *it;
		err = hb_mc_responder_respond(responder, mc, rqst);
		if (err != HB_MC_SUCCESS)
			return err;
	}
	return HB_MC_SUCCESS;
}


int hb_mc_responder_add(hb_mc_responder_t *responder)
{
	if (responders == nullptr)
		responders = new responder_list;

	if (responder->ids == nullptr)
		return HB_MC_FAIL;

	responders->push_front(responder);
	return HB_MC_SUCCESS;
}

int hb_mc_responder_del(hb_mc_responder_t *responder)
{
	if (responders == nullptr)
		return HB_MC_FAIL;

	responders->remove(responder);
	return HB_MC_SUCCESS;
}
