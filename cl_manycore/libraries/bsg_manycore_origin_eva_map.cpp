#include <bsg_manycore_origin_eva_map.h>
#include <bsg_manycore_eva.h>
#include <bsg_manycore_printing.h>
#include <cstring>
#include <cstdlib>

#define ORIGIN_EVA_MAP_MAX_NAME_SIZE 256

int  hb_mc_origin_eva_map_init(hb_mc_eva_map_t *map, hb_mc_coordinate_t origin)
{
	hb_mc_coordinate_t *cpy;
	char name_buffer[ORIGIN_EVA_MAP_MAX_NAME_SIZE];
	char origin_str[32];
	char *name = NULL;
	int r = HB_MC_NOMEM;

	/* check that the map is not null */
	if (!map)
		return HB_MC_INVALID;

	/* initialize the map name */
	snprintf(name_buffer, sizeof(name_buffer), "eva_map @ origin %s",
		 hb_mc_coordinate_to_string(origin, origin_str, sizeof(origin_str)));

	name = strdup(name_buffer);
	if (!name)
		return r;

	/* initialize the origin */
	cpy = (hb_mc_coordinate_t*) malloc(sizeof(*cpy));
	if (!cpy)
		goto cleanup;

	memcpy(cpy, &origin, sizeof(origin));

	/* setup members */
	map->priv = (const void*)cpy;
	map->eva_to_npa = default_eva_to_npa;
	map->npa_to_eva = default_npa_to_eva;
	map->eva_map_name = name;

	r = HB_MC_SUCCESS;
	goto done;

cleanup:
	free(name);

done:
	return r;
}

void hb_mc_origin_eva_map_exit(hb_mc_eva_map_t *map)
{
	hb_mc_coordinate_t *origin;
	const char *name;

	/* check that map is not null */
	if (!map) {
		bsg_pr_err("%s: Calling exit on null map\n", __func__);
		return;
	}

	/* free the private data */
	origin = (hb_mc_coordinate_t*) map->priv;
	if (!origin) {
		bsg_pr_dbg("%s: Calling exit on map with null origin\n",
			   __func__);
	} else {
		free(origin);
		map->priv = NULL;
	}

	/* free the name */
	name = map->eva_map_name;
	if (!name) {
		bsg_pr_dbg("%s: Calling exit on map with null name\n",
			   __func__);
	} else {
		free((void*)name);
		map->eva_map_name = NULL;
	}

	return;
}
