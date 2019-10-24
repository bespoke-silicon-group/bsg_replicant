// Copyright (c) 2019, University of Washington All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list
// of conditions and the following disclaimer.
// 
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// 
// Neither the name of the copyright holder nor the names of its contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <bsg_manycore_origin_eva_map.h>
#include <bsg_manycore_eva.h>
#include <bsg_manycore_printing.h>


#ifdef __cplusplus
#include <cstring>
#include <cstdlib>
#else
#include <string.h>
#include <stdlib.h>
#endif

#define ORIGIN_EVA_MAP_MAX_NAME_SIZE 256


/**
 * Initialize an EVA map for tiles centered at an origin.
 * @param[in] map     An EVA<->NPA map to initialize.
 * @param[in] origin  An origin tile around which the map is centered.
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
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
        map->eva_size = default_eva_size;
        map->eva_map_name = name;

        r = HB_MC_SUCCESS;
        goto done;

 cleanup:
        free(name);

 done:
        return r;
}


/**
 * Cleanup an EVA map for tiles centered at an origin.
 * @param[in] map  An EVA<->NPA map initialized with hb_mc_origin_eva_map_init().
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_origin_eva_map_exit(hb_mc_eva_map_t *map)
{
        hb_mc_coordinate_t *origin;
        const char *name;

        /* check that map is not null */
        if (!map) {
                bsg_pr_err("%s: Calling exit on null map\n", __func__);
                return HB_MC_INVALID;
        }

        /* free the private data */
        origin = (hb_mc_coordinate_t*) map->priv;
        if (!origin) {
                bsg_pr_dbg("%s: Calling exit on map with null origin\n",
                           __func__);
                return HB_MC_INVALID;
        } else {
                free(origin);
                map->priv = NULL;
        }

        /* free the name */
        name = map->eva_map_name;
        if (!name) {
                bsg_pr_dbg("%s: Calling exit on map with null name\n",
                           __func__);
                return HB_MC_INVALID;
        } else {
                free((void*)name);
                map->eva_map_name = NULL;
        }

        return HB_MC_SUCCESS;
}
