#include "bsg_manycore_run_spmd_program.hpp"
#include <bsg_manycore.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_eva.h>
#include <bsg_manycore_loader.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <fstream>

using namespace std;

namespace {
	
hb_mc_manycore_t *mc = nullptr;

std::vector<hb_mc_coordinate_t> create_tile_list(void)
{
	const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
	hb_mc_idx_t base_x, base_y, ceil_x, ceil_y;
	hb_mc_dimension_t dim = hb_mc_config_get_dimension(cfg);
	std::vector<hb_mc_coordinate_t> tiles;
	
	base_x = hb_mc_config_get_vcore_base_x(cfg);
	base_y = hb_mc_config_get_vcore_base_y(cfg);
	ceil_x = hb_mc_dimension_get_x(dim);
	ceil_y = hb_mc_dimension_get_y(dim);

	for (hb_mc_idx_t y = base_y; y < ceil_y; y++)
		for (hb_mc_idx_t x = base_x; x < ceil_x; x++)
			tiles.push_back(hb_mc_coordinate(x,y));

	bsg_pr_dbg("tiles: ");
	for (auto tile : tiles) {
		bsg_pr_dbg("(%d,%d) ",
			   hb_mc_coordinate_get_x(tile),
			   hb_mc_coordinate_get_y(tile));
	}
	bsg_pr_dbg("\n");
	return tiles;
	//return { hb_mc_coordinate(0,1) };
}

std::vector<unsigned char> read_program_data(const char *program_file_name)
{
	std::fstream file;
	struct stat st;
	int err;

	err = stat(program_file_name, &st);
	if (err != 0) {
		bsg_pr_err("failed to stat '%s': %m\n", program_file_name);
		throw err;
	}
	
	file.open(std::string(program_file_name), std::ios::in | std::ios::binary);

	std::vector<unsigned char> program_data(st.st_size, 0);

	file.read((char*)program_data.data(), program_data.size());

	return program_data;	
}

void freeze_tiles(const std::vector<hb_mc_coordinate_t> &tiles)
{
	for (auto tile : tiles) {
		int err = hb_mc_tile_unfreeze(mc, &tile);
		if (err != HB_MC_SUCCESS) {
			bsg_pr_err("failed to unfreeze tile (%" PRId8 ",%" PRId8 "): %s\n",
				   hb_mc_coordinate_get_x(tile),
				   hb_mc_coordinate_get_y(tile),
				   hb_mc_strerror(err));
			throw err;
		}
	}
}

void unfreeze_tiles(const std::vector<hb_mc_coordinate_t> &tiles)
{
	for (auto tile : tiles) {
		int err = hb_mc_tile_unfreeze(mc, &tile);
		if (err != HB_MC_SUCCESS) {
			bsg_pr_err("failed to unfreeze tile (%" PRId8 ",%" PRId8 "): %s\n",
				   hb_mc_coordinate_get_x(tile),
				   hb_mc_coordinate_get_y(tile),
				   hb_mc_strerror(err));
			throw err;
		}
	}
}

int wait_for_finish_packet(void)
{
	while (true) {
		hb_mc_packet_t pkt;
		int err;
		bsg_pr_dbg("Waiting for finish packet\n");
		
		err = hb_mc_manycore_packet_rx(mc, &pkt, HB_MC_FIFO_RX_REQ, -1);
		if (err != HB_MC_SUCCESS) {
			bsg_pr_err("failed to read response packet: %s\n",
				   hb_mc_strerror(err));
		
			throw err;
		}

		char pkt_str[128];
		hb_mc_request_packet_to_string(&pkt.request, pkt_str, sizeof(pkt_str));

		bsg_pr_dbg("received packet %s\n", pkt_str);
		
		switch (hb_mc_request_packet_get_epa(&pkt.request)) {
		case 0xEAD0:
			bsg_pr_dbg("received finish packet\n");
			return 0;
		case 0xEAD8:
			bsg_pr_dbg("received fail packet\n");
			return 1;
		default: break;			
		}
	}
}

void set_tile_origins(hb_mc_coordinate_t origin, std::vector<hb_mc_coordinate_t> &tiles)
{
	for (auto tile : tiles) {
		int err = hb_mc_tile_set_origin(mc, &tile, &origin);
		if (err != HB_MC_SUCCESS) {
			bsg_pr_err("failed to set origin: %s\n", hb_mc_strerror(err));
			throw err;
		}		
	}
}

}

int bsg_manycore_run_spmd_program(const char *program_file_name)
{
	int err, rc = 1;

	
	/* initialize the manycore */
	mc = new hb_mc_manycore_t;
	err = hb_mc_manycore_init(mc, "manycore-spmd", 0);
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("failed to initialize manycore: %s\n",
			   hb_mc_strerror(err));
		return rc;
	}
	
	try {
		/* build the list of tiles */
		auto tiles = create_tile_list();

		/* read in the program data */
		auto program_data = read_program_data(program_file_name);

		/* freeze tiles */
		freeze_tiles(tiles);

		set_tile_origins(hb_mc_coordinate(0,1), tiles);
		
		/* load the binary onto each tile */
		err = hb_mc_loader_load((void*)program_data.data(), program_data.size(),
					mc,
					&default_map,
					tiles.data(),
					tiles.size());
		if (err != HB_MC_SUCCESS) {
			bsg_pr_err("failed to load '%s': %s\n",
				   program_file_name,
				   hb_mc_strerror(err));
			throw err;
		}
		
		/* unfreeze the tiles */
		unfreeze_tiles(tiles);

		/* wait for a finish packet */
		rc = wait_for_finish_packet();		
		
	} catch (int err) {
		rc = 1;
	}

	hb_mc_manycore_exit(mc);	
	delete mc;
	
	return rc;
}
