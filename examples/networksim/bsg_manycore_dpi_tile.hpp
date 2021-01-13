#ifndef __BSG_MANYCORE_DPI_TILE_HPP
#define __BSG_MANYCORE_DPI_TILE_HPP
#include <bsg_manycore_printing.h>
#include <cinttypes>
#include <bsg_manycore.h>
#include <bsg_manycore_npa.h>
#include <bsg_manycore_eva.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_packet.h>

#include <typeinfo>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <queue>
#include <atomic>
#include <utility>
#include <unistd.h>

// These two macro definitions are similar to those in
// bsg_manycore_tile.h. The RTL tile does not define a CSR for
// changing the EVA map, so we do it here!
#define HB_MC_TILE_EPA_CSR_EVA_MAP_OFFSET HB_MC_TILE_EPA_CSR_DRAM_ENABLE_OFFSET + 4
#define HB_MC_TILE_EPA_CSR_EVA_MAP                                      \
        EPA_TILE_CSR_FROM_BYTE_OFFSET(HB_MC_TILE_EPA_CSR_EVA_MAP_OFFSET)

// Add new mappings here:
// Enum for EVA Map. Write this value to the EVA CSR to change EVA maps
typedef enum __bsg_dpi_tile_eva_map_id_t {
        BSG_DPI_TILE_EVA_MAP_ID_DEFAULT = 0,
        BSG_DPI_TILE_EVA_MAP_ID_LINEAR_TLRBRL = 1,
        BSG_DPI_TILE_EVA_MAP_ID_STRIDE_TLRBRL = 2,
        BSG_DPI_TILE_EVA_MAP_ID_MAX = BSG_DPI_TILE_EVA_MAP_ID_STRIDE_TLRBRL + 1,
} bsg_dpi_tile_eva_map_id_t;

hb_mc_eva_map_t * bsg_dpi_tile_eva_maps[] =
        {[BSG_DPI_TILE_EVA_MAP_ID_DEFAULT] = &default_map,
         [BSG_DPI_TILE_EVA_MAP_ID_LINEAR_TLRBRL] = &linear_tlrbrl_map,
         [BSG_DPI_TILE_EVA_MAP_ID_STRIDE_TLRBRL] = &stride_ruche_tlrbrl_map
        };

typedef enum __bsg_dpi_tile_packet_op_t {
        DPI_PACKET_OP_REMOTE_LOAD    = 0,
        DPI_PACKET_OP_REMOTE_STORE   = 1,
} dpi_packet_op_t;

class BsgDpiTile{
public:

        // We maintain a map object for initialized tiles, which
        // requires a hashable type. hb_mc_coordinate_t is not
        // hashable, so we use std::pair instead.
        typedef std::pair<hb_mc_idx_t, hb_mc_idx_t> key_t;
        key_t key(){
                return this->get_key(me);
        }

        static key_t get_key(hb_mc_coordinate_t c){
                return std::make_pair(c.x, c.y);
        }

        static BsgDpiTile* get_tile(key_t key){
                return BsgDpiTile::tiles[key];
        }

        uint32_t set_credit_limit(uint32_t new_limit){
                bsg_pr_info("Tile (X:%d,Y:%d) @ %lu -- %s: "
                            "New credit limit (%u).\n",
                            me.x, me.y, get_cycle(), __func__, new_limit);
                
                uint32_t old = credit_limit;
                if(new_limit > max_credits_available){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "New credit limit (%u) greater than maximum credits available (%u). \n",
                                   me.x, me.y, get_cycle(), __func__, new_limit, max_credits_available);
                        exit(1);
                }
                credit_limit = new_limit;
                return old;
        }

        BsgDpiTile(uint32_t num_tiles_y_p,
                   uint32_t num_tiles_x_p,
                   uint32_t noc_coord_width_y_p,
                   uint32_t noc_coord_width_x_p,
                   uint32_t pod_coord_width_y_p,
                   uint32_t pod_coord_width_x_p,

                   uint32_t icache_entries_p,
                   size_t dmem_size_p,
                   uint32_t addr_width_p,
                   uint32_t data_width_p,

                   uint32_t max_reg_id,
                   uint32_t credit_counter_width_p,

                   uint32_t vcache_sets_p,
                   uint32_t vcache_block_words_p,
                   uint32_t vcache_stripe_words_p,

                   uint32_t host_x_i,
                   uint32_t host_y_i,

                   uint32_t my_x_i,
                   uint32_t my_y_i) :
                // Verilog Parameters
                limits({.x=num_tiles_x_p, .y=num_tiles_y_p}),
                me({.x=my_x_i, .y=my_y_i}),
                host({.x=host_x_i, .y=host_y_i}),
                max_reg_id(max_reg_id),
                dmem_sz(dmem_size_p),
                icache_sz(icache_entries_p * sizeof(icache_entry_t)),
                epa_sz(1<<(addr_width_p + 2)),
                max_credits_available((1 << credit_counter_width_p)),

                // Internal Memories
                icache(new icache_entry_t[icache_entries_p]()),
                dmem(new char[dmem_size_p]()),

                // CSRs
                origin({.x=0, .y=0}),
                frozen(true),
                pc_init(0x1000),
                credit_limit((1 << credit_counter_width_p)),

                // Initial values
                cur_credits_used(0),
                tg_id(0),// Not currently set by the host
                cycle(0){


                // There's probably a better way to do this that
                // provides a single source of truth for the keys of
                // the stat map.
                stats["Fence Write"] = 0;
                stats["Fence Read"] = 0;
                stats["Fence"] = 0;
                stats["Packet RX"] = 0;
                stats["Packet TX"] = 0;
                stats["Stall RegID"] = 0;
                stats["Stall Barrier"] = 0;
                stats["Stall Credit"] = 0;
                stats["Stall !Ready"] = 0;
                stats["Idle"] = 0;

                stats_write_header();

                cur_id.store(-1);
                barrier.store(false);

                bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s: "
                           "Initialization\n",
                           me.x, me.y, get_cycle(), __func__);

                // Create a set with available request IDs
                for(reg_id_t id = 0; id < max_reg_id; ++id){
                        ids_available.insert(id);
                        reorder_buf[id] = {false, new hb_mc_response_packet_t};
                        // Set the default processing cost for each packet to 0.
                        rx_cost[id] = 0;
                }

                bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s:"
                           " %lu IDs available\n",
                           me.x, me.y, get_cycle(), __func__,
                           ids_available.size());

                // Initialize manycore, configuration, and eva_map
                // structs reuse EVA to NPA translation.
                size_t bufsz = 128;
                char *mc_name = new char[bufsz];
                char *map_name = new char[bufsz];

                snprintf(mc_name, bufsz, "Manycore Config @ Tile (X:%d,Y:%d)", me.x, me.y);
                mc.name = mc_name;

                // Technically a CSR, set by the host.
                mc.dram_enabled = 0;

                // The MOST important parts for EVA translation are here:
                mc.config.network_bitwidth_addr = addr_width_p;
                mc.config.network_bitwidth_data = data_width_p;
                mc.config.pods = {.x = 1, .y = 1}; // TODO
                mc.config.noc_coord_width = {.x = noc_coord_width_x_p, .y = noc_coord_width_y_p};
                mc.config.pod_coord_width = {.x = pod_coord_width_x_p, .y = pod_coord_width_y_p};
                mc.config.tile_coord_width = {.x = noc_coord_width_x_p - pod_coord_width_x_p, .y = noc_coord_width_y_p - pod_coord_width_y_p};
                mc.config.pod_shape = limits;
                mc.config.host_interface.x = host.x;
                mc.config.host_interface.y = host.y;
                mc.config.vcache_block_words = vcache_block_words_p;
                mc.config.vcache_stripe_words = vcache_stripe_words_p;

                // Initialize the EVA Map to default
                eva_map.eva_map_name = default_map.eva_map_name;
                eva_map.priv = (const void *)(&origin);
                eva_map.eva_to_npa = default_map.eva_to_npa;
                eva_map.eva_size = default_map.eva_size;
                eva_map.npa_to_eva = default_map.npa_to_eva;

                // Insert this tile into the static tile dictionary.
                tiles[key()] = this;
        }

        uint64_t get_cycle(){
                return cycle;
        }


        ~BsgDpiTile(){
                delete dmem;
                delete icache;
                delete mc.name;
                for(reg_id_t id = 0; id < max_reg_id; ++id){
                        delete reorder_buf[id].second;
                }
                tiles.erase(key());
                if(cur_credits_used != 0)
                        bsg_pr_warn("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                    "%u Memory Operations Still Outstanding \n",
                                    me.x, me.y, get_cycle(), __func__,
                                    cur_credits_used);
                ids_available.clear();
        }

        void execute_cycle(uint32_t network_req_credits_used_i,
                           bool network_req_v_i,
                           const hb_mc_request_packet_t *network_req_i,

                           bool *endpoint_rsp_v_o,
                           hb_mc_response_packet_t *endpoint_rsp_o,
                           bool endpoint_rsp_ready_i,

                           bool network_rsp_v_i,
                           const hb_mc_response_packet_t *network_rsp_i,

                           bool *endpoint_req_v_o,
                           hb_mc_request_packet_t *endpoint_req_o,
                           bool endpoint_req_ready_i
                           ){

                // Set defaults
                *endpoint_rsp_v_o = 0;
                *endpoint_req_v_o = 0;

                if(network_req_credits_used_i > max_credits_available){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Consumed credits (%u) greater than max (%u): \n",
                                   me.x, me.y, get_cycle(), __func__, network_req_credits_used_i, max_credits_available);
                        exit(1);
                }

                cur_credits_used = network_req_credits_used_i;

                // This is the "Main Loop" for a manycore "Tile". It is called
                // on every cycle.

                // This calls functions defined by the class in this order.
                //   1. send_request -- for transmitting one request per cycle
                //   2. execute_request -- for receiving one request AND sending one response per cycle
                //   3. receive_response -- for receiving one response per cycle

                // The order here is important. Tiles should not be able to
                // receive a packet AND THEN transmit a request on the same
                // cycle.

                // Each function is only called when the interface is ready,
                // or there is data available.

                // 1: Transmit one request per cycle (if the interface is ready)

                if(!finished && !frozen && (network_req_credits_used_i != credit_limit) && endpoint_req_ready_i){
                        send_request_internal(endpoint_req_v_o, endpoint_req_o);
                        if(*endpoint_req_v_o)
                                stats["Packet TX"] ++;
                }
                if(!finished && !frozen && (network_req_credits_used_i == credit_limit) && endpoint_req_ready_i){
                        bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s:"
                                    "Tile out of credits.\n",
                                   me.x, me.y, get_cycle(), __func__);
                        stats["Stall Credit"] ++;
                } else if(!finished && !frozen && !endpoint_req_ready_i){
                        stats["Stall !Ready"] ++;
                } else if(!finished && !frozen && endpoint_req_ready_i && !(*endpoint_req_v_o)){
                        stats["Idle"] ++;
                }

                // 2: Respond to an incoming request packet (one per cycle).

                // We assume we're emulating a vanilla tile, so we respond to
                // DMEM/ICACHE/CSR reads/writes automatically. Users can put
                // data in DMEM to have it read by the functions. If the
                // read/write is to DMEM/ICACHE/CSR the code will fill out the
                // response packet automatically. Else, the code will
                // partially fill out the response packet and call a user API
                // function.

                // This MUST happen after transmitting requests so that we
                // don't have zero-latency actions.
                if(network_req_v_i){
                        // Always respond to request packets. (Delay logic is handled in RTL)
                        *endpoint_rsp_v_o = 1;
                        execute_request_internal(network_req_i, endpoint_rsp_o);
                }

                // 3: Receive a network response. This MUST come after
                // responding to network requests on the chance that a tile is
                // receives a DMEM read, and response packet on the same
                // cycle/iteration. In a RISC-V core, the response would go
                // into a register, and then into DMEM, so the DMEM read would
                // happen first. We enforce this ordering here.

                // This MUST happen after transmitting requests so that we
                // don't have zero-cycle actions where a response comes in and
                // a dependent request is sent back out in the same cycle.

                if(network_rsp_v_i){
                        bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Got response packet: %s\n",
                                   me.x, me.y, get_cycle(), __func__,
                                   hb_mc_response_packet_to_string(network_rsp_i, sbuf, sizeof(sbuf)));
                        reg_id_t id = hb_mc_response_packet_get_load_id(network_rsp_i);
                        reorder_buf[id].first = true;
                        *reorder_buf[id].second = *network_rsp_i;

#ifdef BSG_MANYCORE_DPI_TILE_PACKET_RX_OOORDER
                        // Act like it just got sent
                        ids_order.push(id);
#endif
                }

                reg_id_t next = ids_order.front();
                // If packet available, cost is not zero, and a packet wasn't sent
                if(reorder_buf[next].first && (rx_cost[next] != 0) && !(*endpoint_req_v_o)){
                        rx_cost[next]--;
                        stats["Packet RX"] ++;
                }                
                // If packet available, and cost is 0
                if(reorder_buf[next].first && (rx_cost[next] == 0)){
                        network_rsp_i = reorder_buf[next].second;
                        receive_response_internal(network_rsp_i);
                        ids_order.pop();
                        reorder_buf[next].first = false;
                }

                // Finally, Increment the counter
                tick();
        }

private:
        // Verilog parameters
        hb_mc_coordinate_t limits, me, host;
        uint32_t max_reg_id;
        size_t dmem_sz, icache_sz, epa_sz;
        uint32_t max_credits_available;

        // Memories
        // DMEM, and icache are all arrays. Since dmem and
        // icache are sized by initialization parameters from Verilog
        // they are allocated dynamically in the constructor.
        typedef uint32_t icache_entry_t;
        icache_entry_t *icache;
        char *dmem;
        // CSRs (defined in RTL)
        typedef uint32_t csr_entry_t;
        hb_mc_coordinate_t origin;
        csr_entry_t frozen, pc_init;
        uint32_t credit_limit;

        // Internal Variables
        uint32_t cur_credits_used, tg_id;
        // Local Cycle Counter
        uint64_t cycle;


        // Statistics files
        static std::ofstream statfile;
        static std::string filename;// = "dpi_stats.csv";


        // Shared snprintf buffer
        char sbuf[256];

        // User flag that, when set prevents the user methods from being called.
        bool finished = false;

        // This is the hashmap for tracking tiles. It is initialized outside of the class because it is static
        static std::map<key_t, BsgDpiTile*> tiles;

        // Memory network operations have an id number associated with
        // them. Each tile has a limited number of IDs, so we track
        // which are available
        typedef uint8_t reg_id_t;
        std::set<reg_id_t> ids_available;

        // Packet ordering datastructures. Define
        // BSG_MANYCORE_DPI_TILE_PACKET_RX_OOORDER in the cpp file if
        // you want packets to be recieved and processed in the order
        // they were returned, not in the order they were sent
        std::queue<reg_id_t> ids_order;
        typedef std::pair<bool, hb_mc_response_packet_t*> resv_t;
        std::map<reg_id_t, resv_t> reorder_buf;
        std::map<reg_id_t, unsigned int> rx_cost;

        reg_id_t get_available_id(){
                return *(ids_available.begin());
        }


        // Configuration Structs
        hb_mc_manycore_t mc;
        hb_mc_eva_map_t eva_map;

        // Utility functions
        bool is_origin(){
                return (origin.x == me.x) && (origin.y == me.y);
        }

        void tick(){
                cycle++;
        }

        bool fence_read(){
                bool ret = (ids_available.size() != max_reg_id);
                stats["Fence Read"] += ret;
                return ret;
        }

        // If we want a true write-fence we need to track outstanding
        // reads separately from outstanding writes -- and the RTL
        // needs to adapt to respond when write requests return.  This
        // is a mild hack. It only makes sure that all outstanding
        // credits are for reads.
        bool fence_write(){
                bool ret = cur_credits_used == (max_reg_id - ids_available.size());
                stats["Fence Write"] += ret;
                return ret;
        }

        bool fence(){
                bool ret = (cur_credits_used != 0);
                stats["Fence"] += ret;
                return ret;
        }


        // Inter-Tile barrier functionality.
        // barrier indicates this tile is at the barrier
        std::atomic<bool> barrier;
        // cur_id tracks the current barrier iteration (so that tiles can all reach the same barrier)
        std::atomic<int> cur_id;

        bool wait_all_done(hb_mc_coordinate_t tg, int barrier_id){
                bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s: "
                           "Origin Tile Barrier barrier_id %d\n",
                           me.x, me.y, get_cycle(), __func__, barrier_id);
                BsgDpiTile::key_t k;
                BsgDpiTile *t;
                for(hb_mc_idx_t x_i = me.x; x_i < me.x + tg.x; ++x_i){
                        for(hb_mc_idx_t y_i = me.y; y_i < me.y + tg.y; ++y_i){
                                k = BsgDpiTile::get_key({.x=x_i, .y=y_i});
                                t = BsgDpiTile::get_tile(k);
                                int their_cur_id = t->cur_id.load();
                                bool their_barrier = t->barrier.load();
                                if(their_cur_id == barrier_id - 1)
                                        return true;
                                else if(their_cur_id != barrier_id){
                                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                                   "Unexpected barrier barrier_id value "
                                                   "Got %d when checking Tile at (X:%d,Y:%d)\n",
                                                   me.x, me.y, get_cycle(), __func__, their_cur_id, x_i, y_i);
                                        exit(1);
                                }
                        }
                }

                for(hb_mc_idx_t x_i = me.x; x_i < me.x + tg.x; ++x_i){
                        for(hb_mc_idx_t y_i = me.y; y_i < me.y + tg.y; ++y_i){
                                k = BsgDpiTile::get_key({.x=x_i, .y=y_i});
                                t = BsgDpiTile::get_tile(k);
                                t->barrier.store(false);
                        }
                }
                bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s: Origin Tile Barrier finished barrier_id %d\n",
                           me.x, me.y, get_cycle(), __func__, barrier_id);
                return false;
        }

        bool wait_at_barrier(int barrier_id, hb_mc_coordinate_t tg){
                int my_cur_id = cur_id.load();
                bool my_barrier = barrier.load();
                bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s: "
                           "Origin: %d Barrier Barrier_Id %d (CUR_ID: %d)\n",
                           me.x, me.y, get_cycle(), __func__,
                           is_origin(), barrier_id, my_cur_id);
                if(my_cur_id > barrier_id){
                        return false;
                } else if((my_cur_id == barrier_id) && is_origin() && my_barrier) {
                        cur_id.store(barrier_id);
                        // Check all TG subordinates
                        bool barrier = wait_all_done(tg, barrier_id);
                        if(barrier)
                                stats["Stall Barrier"] ++;
                        return barrier;
                } else if((my_cur_id == barrier_id)){
                        if(my_barrier)
                                stats["Stall Barrier"] ++;
                        return my_barrier;
                } else if((my_cur_id == barrier_id - 1)){
                        barrier.store(true);
                        cur_id.store(barrier_id);
                        stats["Stall Barrier"] ++;
                        return true;
                }
                bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                           "Unexpected barrier barrier_id value %d\n",
                           me.x, me.y, get_cycle(), __func__, my_cur_id);
                exit(1);
                return false;
        }

        // User defined function, called by send_request_internal
        void send_request(bool *req_v_o, hb_mc_request_packet_t *req_o);
        void send_request_internal(bool *req_v_o,
                                   hb_mc_request_packet_t *req_o){

                if(!ids_available.empty()){
                        send_request(req_v_o, req_o);
                } else {
                        stats["Stall RegID"] ++;

                        bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Out of request ids\n",
                                    me.x, me.y, get_cycle(), __func__);
                }

                if(*req_v_o && (hb_mc_request_packet_get_op(req_o) == HB_MC_PACKET_OP_REMOTE_LOAD)){
                        reg_id_t id = hb_mc_request_packet_get_load_id(req_o);

                        if(id >= max_reg_id){
                                bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                           "Invalid ID %d. Packet: %s\n",
                                           me.x, me.y, get_cycle(), __func__, id,
                                           hb_mc_request_packet_to_string(req_o, sbuf, sizeof(sbuf)));
                                exit(1);
                        }

                        if(ids_available.count(id) == 0){
                                bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                           "ID %d already in use. Packet: %s\n",
                                           me.x, me.y, get_cycle(), __func__, id,
                                           hb_mc_request_packet_to_string(req_o, sbuf, sizeof(sbuf)));
                                exit(1);
                        }

                        bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "%s\n",
                                   me.x, me.y, get_cycle(), __func__,
                                   hb_mc_request_packet_to_string(req_o, sbuf, sizeof(sbuf)));

                        ids_available.erase(id);

#ifndef BSG_MANYCORE_DPI_TILE_PACKET_RX_OOORDER
                        ids_order.push(id);
#endif
                }
        }

        // User defined function, called by receive_response_internal
        // Future feature: Automatically store responses in a
        // location, or in a queue (for in-order processing???)
        // typedef std::tuple<hb_mc_epa_t,
        // int, void*> load_info_t; std::map<reg_id_t, load_info_t>
        // load_info;
        void receive_response(const hb_mc_response_packet_t *network_rsp_i);
        void receive_response_internal(const hb_mc_response_packet_t *rsp_i){
                reg_id_t id = hb_mc_response_packet_get_load_id(rsp_i);

                if(id >= max_reg_id){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Invalid ID %d. Packet: %s\n",
                                   me.x, me.y, get_cycle(), "rsp", id,
                                   hb_mc_response_packet_to_string(rsp_i, sbuf, sizeof(sbuf)));
                        exit(1);
                }

                if(ids_available.count(id) != 0){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Unexpected response for ID %d. Packet: %s\n",
                                   me.x, me.y, get_cycle(), __func__, id,
                                   hb_mc_response_packet_to_string(rsp_i, sbuf, sizeof(sbuf)));
                        exit(1);
                }

                receive_response(rsp_i);

                // Print the response packet (when debugging)
                bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s: "
                           "%s\n",
                           me.x, me.y, get_cycle(), __func__,
                           hb_mc_response_packet_to_string(rsp_i, sbuf, sizeof(sbuf)));

                ids_available.insert(id);
        }

        // User defined function, called by execute_request_internal
        void execute_request(const hb_mc_request_packet_t *req, hb_mc_response_packet_t *rsp);
        void execute_request_internal(const hb_mc_request_packet_t *req_i, hb_mc_response_packet_t *rsp_o){

                // Print the request packet (when debugging)
                bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s: "
                           "%s\n",
                           me.x, me.y, get_cycle(), __func__,
                           hb_mc_request_packet_to_string(req_i, sbuf, sizeof(sbuf)));

                // Fill in the response packet to make life easier
                hb_mc_response_packet_fill(rsp_o, req_i);

                execute_request(req_i, rsp_o);

                // Print the response packet (when debugging)
                bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s: "
                           "%s\n",
                           me.x, me.y, get_cycle(), __func__,
                           hb_mc_response_packet_to_string(rsp_o, sbuf, sizeof(sbuf)));
        }

        // EPA Validation functions
        bool epa_is_dmem(hb_mc_epa_t addr){
                return epa_in_range(addr, HB_MC_TILE_EPA_DMEM_BASE, dmem_sz);
        }

        bool epa_is_icache(hb_mc_epa_t addr){
                return epa_in_range(addr, HB_MC_TILE_EPA_ICACHE_BASE, icache_sz);
        }

        bool epa_is_csr(hb_mc_epa_t addr){
                return epa_in_range(addr, HB_MC_TILE_EPA_CSR_BASE, HB_MC_TILE_EPA_CSR_EVA_MAP_OFFSET + 4);
        }

        bool epa_is_epa(hb_mc_epa_t addr){
                return epa_in_range(addr, 0, epa_sz);
        }

        bool epa_in_range(hb_mc_epa_t addr, hb_mc_epa_t base, size_t sz){
                bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s: "
                           "%u, %lu\n",
                           me.x, me.y, get_cycle(), __func__,
                           EPA_FROM_BASE_AND_OFFSET(base, 0), EPA_FROM_BASE_AND_OFFSET(base, sz));
                return ((addr >= EPA_FROM_BASE_AND_OFFSET(base, 0)) &&
                        (addr < EPA_FROM_BASE_AND_OFFSET(base, sz)));
        }

        // Can be called by user to handle requests. Mimics tile behavior
        void default_request_handler(const hb_mc_request_packet_t *req,
                                     hb_mc_response_packet_t *rsp){
                hb_mc_epa_t addr = hb_mc_request_packet_get_epa(req);
                if (epa_is_dmem(addr))
                        execute_request_dmem(req, rsp);
                else if (epa_is_icache(addr))
                        execute_request_icache(req, rsp);
                else if (epa_is_csr(addr))
                        execute_request_csr(req, rsp);
                else if(!epa_is_epa(addr)){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Invalid EPA. Packet: %s\n",
                                   me.x, me.y, get_cycle(), __func__,
                                   hb_mc_request_packet_to_string(req, sbuf, sizeof(sbuf)));
                        exit(1);
                } else {
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Valid, but unmapped EPA. Packet: %s\n",
                                   me.x, me.y, get_cycle(), __func__,
                                   hb_mc_request_packet_to_string(req, sbuf, sizeof(sbuf)));
                        exit(1);
                }
        }

        void execute_request_read(void *buf, size_t base, hb_mc_epa_t epa,
                                  hb_mc_request_packet_load_info_t info, uint32_t *data){

                size_t offset = OFFSET_FROM_BASE_AND_EPA(base, epa);
                execute_request_read(buf, offset, info, data);
        }

        void execute_request_read(void *buf,
                                  size_t offset,
                                  hb_mc_request_packet_load_info_t info,
                                  uint32_t *data){
                char *p;
                if(info.is_hex_op && info.is_byte_op){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Read request cannot have hex AND byte set.\n",
                                   me.x, me.y, get_cycle(), __func__);
                        exit(1);
                }

                size_t sz =
                        4 * (!info.is_byte_op && !info.is_hex_op) + // Word op
                        2 * (info.is_hex_op > 0) + // Half op
                        1 * (info.is_byte_op > 0); // byte op

                offset += info.part_sel;
                if(offset % sz){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Read request unaligned. Address: %lx, Size: %lu\n",
                                   me.x, me.y, get_cycle(), __func__,
                                   offset, sz);
                        exit(1);
                }

                p = get_ptr(buf, offset, HB_MC_PACKET_REQUEST_MASK_BYTE_0);

                switch(sz){
                case(sizeof(uint32_t)):
                        *data = *reinterpret_cast<uint32_t *>(p);
                        break;
                case(sizeof(uint16_t)):
                        *data = *reinterpret_cast<uint16_t *>(p);
                        break;
                case(sizeof(uint8_t)):
                        *data = *reinterpret_cast<uint8_t *>(p);
                        break;
                }
        }

        void execute_request_write(void *buf,
                                   size_t base,
                                   hb_mc_epa_t epa,
                                   hb_mc_packet_mask_t mask,
                                   uint32_t data){
                size_t offset = OFFSET_FROM_BASE_AND_EPA(base, epa);

                execute_request_write(buf, offset, mask, data);
        }

        void execute_request_write(void *buf,
                                   size_t offset,
                                   hb_mc_packet_mask_t mask,
                                   uint32_t data){
                char *p = get_ptr(buf, offset, mask);

                switch(mask){
                case(HB_MC_PACKET_REQUEST_MASK_WORD):
                        *reinterpret_cast<uint32_t *>(p) = data;
                        break;
                case(HB_MC_PACKET_REQUEST_MASK_SHORT_0):
                        *reinterpret_cast<uint16_t *>(p) = data;
                        break;
                case(HB_MC_PACKET_REQUEST_MASK_SHORT_1):
                        *reinterpret_cast<uint16_t *>(p) = data >> (1 * 8 * sizeof(uint16_t));
                        break;
                case(HB_MC_PACKET_REQUEST_MASK_BYTE_0):
                        *reinterpret_cast<uint8_t *>(p) = data;
                        break;
                case(HB_MC_PACKET_REQUEST_MASK_BYTE_1):
                        *reinterpret_cast<uint8_t *>(p) = data >> (1 * 8 * sizeof(uint8_t));
                        break;
                case(HB_MC_PACKET_REQUEST_MASK_BYTE_2):
                        *reinterpret_cast<uint8_t *>(p) = data >> (2 * 8 * sizeof(uint8_t));
                        break;
                case(HB_MC_PACKET_REQUEST_MASK_BYTE_3):
                        *reinterpret_cast<uint8_t *>(p) = data >> (3 * 8 * sizeof(uint8_t));
                        break;
                default:
                        bsg_pr_warn("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                    "Invalid mask value. Mask: %x\n",
                                    me.x, me.y, get_cycle(), __func__, mask);
                }
        }

        char* get_ptr(void *buf,
                      size_t offset,
                      hb_mc_packet_mask_t mask){

                char *p = reinterpret_cast<char *>(buf);

                switch(mask){
                case(HB_MC_PACKET_REQUEST_MASK_WORD):
                case(HB_MC_PACKET_REQUEST_MASK_SHORT_0):
                case(HB_MC_PACKET_REQUEST_MASK_BYTE_0):
                        return (p + offset);
                case(HB_MC_PACKET_REQUEST_MASK_BYTE_1):
                        return (p + offset + sizeof(uint8_t));
                case(HB_MC_PACKET_REQUEST_MASK_SHORT_1):
                case(HB_MC_PACKET_REQUEST_MASK_BYTE_2):
                        return (p + offset + sizeof(uint16_t));
                case(HB_MC_PACKET_REQUEST_MASK_BYTE_3):
                        return (p + offset + 3 *sizeof(uint8_t));
                default:
                        bsg_pr_warn("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                    "Invalid mask value. Mask: %x\n",
                                    me.x, me.y, get_cycle(), __func__,
                                    mask);
                        return p;
                }
        }

        void execute_request_buf(const hb_mc_request_packet_t *req,
                                 hb_mc_response_packet_t *rsp,
                                 void *buf,
                                 size_t base){

                hb_mc_packet_op_t op = static_cast<hb_mc_packet_op_t>(hb_mc_request_packet_get_op(req));
                hb_mc_epa_t epa = hb_mc_request_packet_get_epa(req);

                bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s: "
                           "%s\n",
                           me.x, me.y, get_cycle(), __func__,
                           hb_mc_request_packet_to_string(req, sbuf, sizeof(sbuf)));

                uint32_t data;
                if ((op == HB_MC_PACKET_OP_REMOTE_LOAD)){
                        execute_request_read(buf, base, epa, hb_mc_request_packet_get_load_info(req), &data);
                        hb_mc_response_packet_set_data(rsp, data);
                } else if ((op == HB_MC_PACKET_OP_REMOTE_STORE) ||
                           (op == HB_MC_PACKET_OP_REMOTE_SW)){
                        hb_mc_packet_mask_t mask = static_cast<hb_mc_packet_mask_t>(hb_mc_request_packet_get_mask(req));
                        data = hb_mc_request_packet_get_data(req);
                        execute_request_write(buf, base, epa, mask, data);
                } else {
                        // All others unsupported
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Invalid Op. Packet: %s\n",
                                   me.x, me.y, get_cycle(), __func__,
                                   hb_mc_request_packet_to_string(req, sbuf, sizeof(sbuf)));
                        exit(1);
                }
        }

        void execute_request_icache(const hb_mc_request_packet_t *req,
                                    hb_mc_response_packet_t *rsp){
                hb_mc_packet_op_t op = static_cast<hb_mc_packet_op_t>(hb_mc_request_packet_get_op(req));
                hb_mc_packet_mask_t mask = static_cast<hb_mc_packet_mask_t>(hb_mc_request_packet_get_mask(req));
                hb_mc_request_packet_load_info_t info = hb_mc_request_packet_get_load_info(req);
                bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s: "
                           "%s\n",
                           me.x, me.y, get_cycle(), __func__,
                           hb_mc_request_packet_to_string(req, sbuf, sizeof(sbuf)));

                // Only word-level operations are supported
                if (!((op == HB_MC_PACKET_OP_REMOTE_SW) ||
                      (op == HB_MC_PACKET_OP_REMOTE_LOAD && !info.is_byte_op && !info.is_hex_op))){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Invalid operation. Only supports word-level operations. Packet: %s\n",
                                   me.x, me.y, get_cycle(), __func__,
                                   hb_mc_request_packet_to_string(req, sbuf, sizeof(sbuf)));
                        exit(1);
                }

                if((op == HB_MC_PACKET_OP_REMOTE_LOAD)){
                        bsg_pr_warn("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                    "I-Cache Reads are not supported in RTL. Packet: %s\n",
                                    me.x, me.y, get_cycle(), __func__,
                                    hb_mc_request_packet_to_string(req, sbuf, sizeof(sbuf)));
                }

                execute_request_buf(req, rsp, icache, HB_MC_TILE_EPA_ICACHE_BASE);
        }

        void execute_request_dmem(const hb_mc_request_packet_t *req,
                                  hb_mc_response_packet_t *rsp){
                bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s: "
                           "%s\n",
                           me.x, me.y, get_cycle(), __func__,
                           hb_mc_request_packet_to_string(req, sbuf, sizeof(sbuf)));

                execute_request_buf(req, rsp, dmem, HB_MC_TILE_EPA_DMEM_BASE);
        }

        void execute_request_csr(const hb_mc_request_packet_t *req,
                                 hb_mc_response_packet_t *rsp){
                hb_mc_packet_mask_t mask = static_cast<hb_mc_packet_mask_t>(hb_mc_request_packet_get_mask(req));

                hb_mc_epa_t addr = hb_mc_request_packet_get_epa(req);
                uint32_t data = hb_mc_request_packet_get_data(req);
                hb_mc_packet_op_t op = static_cast<hb_mc_packet_op_t>(hb_mc_request_packet_get_op(req));

                bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s: "
                           "%s\n",
                           me.x, me.y, get_cycle(), __func__,
                           hb_mc_request_packet_to_string(req, sbuf, sizeof(sbuf)));

                if(mask != HB_MC_PACKET_REQUEST_MASK_WORD){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Invalid Mask. CSRs only support word-level operations. Packet: %s\n",
                                   me.x, me.y, get_cycle(), __func__,
                                   hb_mc_request_packet_to_string(req, sbuf, sizeof(sbuf)));
                        exit(1);
                }

                if ((op != HB_MC_PACKET_OP_REMOTE_SW)){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Invalid Op. CSRs only support word write operations. Packet: %s\n",
                                   me.x, me.y, get_cycle(), __func__,
                                   hb_mc_request_packet_to_string(req, sbuf, sizeof(sbuf)));
                        exit(1);
                }

                const char * csr_name;
                // Execute the CSR Request
                switch(addr){
                case HB_MC_TILE_EPA_CSR_FREEZE:
                        frozen = data;
                        csr_name = "CSR Freeze/Unfreeze";
                        break;
                case HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X:
                        origin.x = data;
                        csr_name = "CSR Origin X";
                        break;
                case HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y:
                        origin.y = data;
                        csr_name = "CSR Origin Y";
                        break;
                case HB_MC_TILE_EPA_CSR_PC_INIT_VALUE:
                        pc_init = data;
                        csr_name = "CSR PC Init";
                        break;
                case HB_MC_TILE_EPA_CSR_DRAM_ENABLE:
                        mc.dram_enabled = 1;
                        csr_name = "CSR DRAM Enable";
                        break;
                case HB_MC_TILE_EPA_CSR_EVA_MAP:
                        if(data >= BSG_DPI_TILE_EVA_MAP_ID_MAX){
                                bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                           "Unknown EVA Map %u\n",
                                           me.x, me.y, get_cycle(), __func__, data);
                                exit(1);
                        }
                        bsg_pr_info("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                    "Set EVA map to %s\n",
                                    me.x, me.y, get_cycle(), __func__,
                                    bsg_dpi_tile_eva_maps[data]->eva_map_name),
                        eva_map.eva_map_name = bsg_dpi_tile_eva_maps[data]->eva_map_name;
                        eva_map.eva_to_npa = bsg_dpi_tile_eva_maps[data]->eva_to_npa;
                        eva_map.eva_size = bsg_dpi_tile_eva_maps[data]->eva_size;
                        eva_map.npa_to_eva = bsg_dpi_tile_eva_maps[data]->npa_to_eva;
                        csr_name = "CSR EVA Map";
                        break;
                default:
                        csr_name = "CSR EPA Unknown";
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "%s -- Packet: %s\n",
                                   me.x, me.y, get_cycle(), __func__, csr_name,
                                   hb_mc_request_packet_to_string(req, sbuf, sizeof(sbuf)));
                        exit(1);
                }
                bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s: "
                           "Write %s with %u\n",
                           me.x, me.y, get_cycle(), __func__, csr_name, data);
        }


        std::map<const std::string, uint32_t> stats;
        void stats_write(uint32_t payload){
                statfile << get_cycle() << "," << payload << "," << me.x << "," << me.y;
                for (auto it = stats.begin(); it!=stats.end(); ++it){
                        statfile << ",";
                        statfile << it->second;
                }
                statfile << std::endl;
                statfile.flush();
        }

        void stats_write_header(){
                if(statfile.tellp() == 0){
                        statfile << "Cycle,Payload,X,Y";
                        for (auto it = stats.begin(); it!=stats.end(); ++it){
                                statfile << ",";
                                statfile << it->first;
                        }
                        statfile << std::endl;
                        statfile.flush();
                }
        }

        void set_packet_rx_cost(const hb_mc_request_packet_t *req, unsigned int cost){
                hb_mc_packet_op_t op = static_cast<hb_mc_packet_op_t>(hb_mc_request_packet_get_op(req));
                if(op != HB_MC_PACKET_OP_REMOTE_LOAD){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Invalid Op. Only read packets can have a cost. Packet: %s\n",
                                   me.x, me.y, get_cycle(), __func__,
                                   hb_mc_request_packet_to_string(req, sbuf, sizeof(sbuf)));
                        exit(1);
                }
                reg_id_t id = hb_mc_request_packet_get_load_id(req);
                if(rx_cost[id] > 0){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Packet RX cost has already been set.\n",
                                   me.x, me.y, get_cycle(), __func__);
                        exit(1);
                }
                rx_cost[id] = cost;
        }

        // All of the get_packet functions produce a valid network
        // request packet. At the lowest level, users can specify all
        // the packet fields (get_packet_from_npa), and at the highest
        // level, the fields are pre-defined based on the
        // functionality of the packet (e.g. get_packet_finish -- a
        // finish packet for the host).

        // Overloading allows read and write packets to be formatted
        // based on the number of arguments.

        // Read call chain
        template<typename T>
        bool get_packet_host(hb_mc_request_packet_t *req, hb_mc_epa_t addr){
                bsg_pr_warn("Tile (X:%d,Y:%d) @ %lu -- %s: "
                            "Reads to the host may not be supported\n",
                            me.x, me.y, get_cycle(), __func__);
                return get_packet_from_xy_epa<T>(req, host, addr);
        }

        template<typename T>
        bool get_packet_from_xy_epa(hb_mc_request_packet_t *req, hb_mc_coordinate_t dest, hb_mc_epa_t addr){
                reg_id_t id = get_available_id();
                return get_packet_from_xy_epa<T>(req, dest, addr, id);
        }

        template<typename T>
        bool get_packet_from_xy_epa(hb_mc_request_packet_t *req, hb_mc_coordinate_t dest, hb_mc_epa_t addr, reg_id_t id){
                hb_mc_npa_t npa = {.x=dest.x, .y=dest.y, .epa=addr};
                return get_packet_from_npa<T>(req, &npa, id);
        }

        template<typename T>
        bool get_packet_from_npa(hb_mc_request_packet_t *req, hb_mc_npa_t *npa, reg_id_t id){
                return get_packet_from_npa<T>(req, npa, 0, id, DPI_PACKET_OP_REMOTE_LOAD);
        }

        template<typename T>
        bool get_packet_from_eva(hb_mc_request_packet_t *req, hb_mc_eva_t addr){
                reg_id_t id = get_available_id();
                return get_packet_from_eva<T>(req, addr, 0, id, DPI_PACKET_OP_REMOTE_LOAD);
        }

        template<typename T>
        bool get_packet_from_eva(hb_mc_request_packet_t *req, hb_mc_eva_t addr, reg_id_t id){
                return get_packet_from_eva<T>(req, addr, 0, id, DPI_PACKET_OP_REMOTE_LOAD);
        }

        // Write Call Chain
        bool get_packet_finish(hb_mc_request_packet_t *req){
                return get_packet_host(req, HB_MC_HOST_EPA_FINISH, 1);
        }

        bool get_packet_fail(hb_mc_request_packet_t *req){
                return get_packet_host(req, HB_MC_HOST_EPA_FAIL, 1);
        }

        bool get_packet_stat_kernel_start(hb_mc_request_packet_t *req){
                return get_packet_stat(req, 0, false, true);
        }

        bool get_packet_stat_kernel_end(hb_mc_request_packet_t *req){
                return get_packet_stat(req, 0, true, true);
        }

        bool get_packet_stat_tag_start(hb_mc_request_packet_t *req, uint32_t tag){
                return get_packet_stat(req, tag, false, false);
        }

        bool get_packet_stat_tag_end(hb_mc_request_packet_t *req, uint32_t tag){
                return get_packet_stat(req, tag, true, false);
        }

        bool get_packet_stat(hb_mc_request_packet_t *req, uint32_t tag, bool end, bool kernel){
                uint32_t payload = format_stat_payload(tag, kernel, end);
                stats_write(payload);
                return get_packet_host(req, HB_MC_HOST_EPA_STATS, payload);
        }

        template<typename T>
        bool get_packet_host(hb_mc_request_packet_t *req, hb_mc_epa_t addr, T data){
                return get_packet_from_xy_epa<T>(req, host, addr, data);
        }

        template<typename T>
        bool get_packet_from_xy_epa(hb_mc_request_packet_t *req, hb_mc_coordinate_t dest, hb_mc_epa_t addr, T data){
                reg_id_t id = get_available_id();
                return get_packet_from_xy_epa<T>(req, dest, addr, data, id);
        }

        template<typename T>
        bool get_packet_from_xy_epa(hb_mc_request_packet_t *req, hb_mc_coordinate_t dest, hb_mc_epa_t addr, T data, reg_id_t id){
                hb_mc_npa_t npa = {.x=dest.x, .y=dest.y, .epa=addr};
                return get_packet_from_npa<T>(req, &npa, data, id);
        }

        template<typename T>
        bool get_packet_from_npa(hb_mc_request_packet_t *req, hb_mc_npa_t *npa, T data, reg_id_t id){
                return get_packet_from_npa<T>(req, npa, data, id, DPI_PACKET_OP_REMOTE_STORE);
        }

        template<typename T>
        bool get_packet_from_eva(hb_mc_request_packet_t *req, hb_mc_eva_t addr, T data){
                reg_id_t id = get_available_id();
                return get_packet_from_eva<T>(req, addr, data, id, DPI_PACKET_OP_REMOTE_STORE);
        }

        template<typename T>
        bool get_packet_from_eva(hb_mc_request_packet_t *req, hb_mc_eva_t addr, T data, reg_id_t id){
                return get_packet_from_eva<T>(req, addr, data, id, DPI_PACKET_OP_REMOTE_STORE);
        }

        // Merge call chains
        template<typename T>
        bool get_packet_from_eva(hb_mc_request_packet_t *req, hb_mc_eva_t addr, T data, reg_id_t id, dpi_packet_op_t op){
                static_assert(std::is_integral<T>::value, "Invalid type. get_packet_from_eva only supports integral types (char, short, int, etc)\n");

                hb_mc_npa_t npa;
                size_t sz;
                int err = hb_mc_eva_to_npa(&mc, &eva_map, &me, &addr, &npa, &sz);
                if(err != HB_MC_SUCCESS){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Invalid EVA: %u\n",
                                   me.x, me.y, get_cycle(), __func__, addr);
                        exit(1);
                }
                return get_packet_from_npa(req, &npa, data, id, op);
        }

        template <typename T>
        bool get_packet_from_npa(hb_mc_request_packet_t *req, hb_mc_npa_t *npa, T _data, reg_id_t id, dpi_packet_op_t op){
                hb_mc_epa_t epa = hb_mc_npa_get_epa(npa);
                uint8_t shift = epa & 0x3, sz = sizeof(T);
                hb_mc_packet_mask_t mask;
                uint32_t data = static_cast<uint32_t>(_data);
                int err = hb_mc_manycore_epa_check_alignment(&epa, sz);

                if(err == HB_MC_UNALIGNED){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Unaligned NPA:%s\n",
                                   me.x, me.y, get_cycle(), __func__,
                                   hb_mc_npa_to_string(npa, sbuf, sizeof(sbuf)));
                        exit(1);
                }

                if(err == HB_MC_INVALID){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Invalid size %lu, NPA:%s\n",
                                   me.x, me.y, get_cycle(), __func__, sizeof(T),
                                   hb_mc_npa_to_string(npa, sbuf, sizeof(sbuf)));
                        exit(1);
                }

                switch(sz){
                case(sizeof(char)):
                        mask = HB_MC_PACKET_REQUEST_MASK_BYTE;
                        break;
                case(sizeof(short)):
                        mask = HB_MC_PACKET_REQUEST_MASK_SHORT;
                        break;
                case(sizeof(int)):
                        mask = HB_MC_PACKET_REQUEST_MASK_WORD;
                        break;
                }

                mask = static_cast<hb_mc_packet_mask_t>(mask << shift);
                data = (data << shift);

                return get_packet_from_npa<T>(req, npa, data, id, op, mask);
        }

        template <typename T>
        bool get_packet_from_npa(hb_mc_request_packet_t *req, hb_mc_npa_t *npa, uint32_t data, reg_id_t id, dpi_packet_op_t dpi_op, hb_mc_packet_mask_t mask){
                // Default to remote load
                hb_mc_packet_op_t mc_op = HB_MC_PACKET_OP_REMOTE_LOAD;
                uint8_t sz = sizeof(T);                
                if(sz > sizeof(int)){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Invalid datatype for request. "
                                   "%s\n",
                                   me.x, me.y, get_cycle(), __func__,
                                   typeid(T).name());
                        exit(1);
                }

                if(dpi_op == DPI_PACKET_OP_REMOTE_STORE){
                        switch(sz){
                        case(sizeof(char)):
                        case(sizeof(short)):
                                mc_op = HB_MC_PACKET_OP_REMOTE_STORE;
                        break;
                        case(sizeof(int)):
                                mc_op = HB_MC_PACKET_OP_REMOTE_SW;
                                break;
                        }
                }
                return get_packet_from_npa(req, npa, data, id, mc_op, mask);
        }

        void get_payload_encoding(uint32_t *payload, hb_mc_packet_mask_t mask, reg_id_t id){
                switch(mask){
                case(HB_MC_PACKET_REQUEST_MASK_BYTE_1):
                case(HB_MC_PACKET_REQUEST_MASK_BYTE_2):
                case(HB_MC_PACKET_REQUEST_MASK_BYTE_3):
                case(HB_MC_PACKET_REQUEST_MASK_SHORT_1):
                        // Store in Byte 0
                        *payload |= id;
                        return;
                case(HB_MC_PACKET_REQUEST_MASK_BYTE_0):
                        // Store in Byte 1
                        *payload |= (id << 8);
                        return;
                case(HB_MC_PACKET_REQUEST_MASK_SHORT_0):
                        // Store in Byte 2
                        *payload |= (id << 16);
                        return;
                case(HB_MC_PACKET_REQUEST_MASK_WORD):
                default:
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Invalid request to encode payload. "
                                   "Payload: %x, Mask: %x, ID: %d\n",
                                   me.x, me.y, get_cycle(), __func__,
                                   *payload, mask, id);
                        exit(1);
                }
        }

        bool get_packet_from_npa(hb_mc_request_packet_t *req_o, hb_mc_npa_t *npa, uint32_t data, reg_id_t id, hb_mc_packet_op_t op, hb_mc_packet_mask_t mask){
                hb_mc_request_packet_set_x_dst(req_o, hb_mc_npa_get_x(npa));
                hb_mc_request_packet_set_y_dst(req_o, hb_mc_npa_get_y(npa));
                hb_mc_request_packet_set_x_src(req_o, me.x);
                hb_mc_request_packet_set_y_src(req_o, me.y);
                hb_mc_request_packet_set_epa(req_o, hb_mc_npa_get_epa(npa));
                hb_mc_request_packet_set_op(req_o, op);
                switch(op){
                case HB_MC_PACKET_OP_REMOTE_LOAD:
                        hb_mc_request_packet_set_load_id(req_o, id);
                        break;
                case HB_MC_PACKET_OP_REMOTE_SW:
                case HB_MC_PACKET_OP_REMOTE_AMOSWAP:
                case HB_MC_PACKET_OP_REMOTE_AMOADD:
                case HB_MC_PACKET_OP_REMOTE_AMOXOR:
                case HB_MC_PACKET_OP_REMOTE_AMOAND:
                case HB_MC_PACKET_OP_REMOTE_AMOMIN:
                case HB_MC_PACKET_OP_REMOTE_AMOMAX:
                case HB_MC_PACKET_OP_REMOTE_AMOMINU:
                case HB_MC_PACKET_OP_REMOTE_AMOMAXU:
                        // Load ID field is load ID, mask is implicit with word-level ops
                        // Data field is not repurposed
                        hb_mc_request_packet_set_load_id(req_o, id);
                        hb_mc_request_packet_set_data(req_o, data);
                        break;
                case HB_MC_PACKET_OP_REMOTE_STORE:
                        // load_id is mask field (hidden in API), load_id is stored in payload
                        hb_mc_request_packet_set_mask(req_o, mask);
                        get_payload_encoding(&data, mask, id);
                        hb_mc_request_packet_set_data(req_o, data);
                        break;
                default:
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Invalid operation. Op: %x\n",
                                   me.x, me.y, get_cycle(), __func__, op);
                        exit(1);
                        break;
                }
                return true;
        }

        typedef union _stat_payload_t {
                struct __attribute__ ((packed)) {
                        uint32_t tag : 4;
                        uint32_t tg_id : 14;
                        hb_mc_idx_t x : 6;
                        hb_mc_idx_t y : 6;
                        bool end : 1;
                        bool kernel : 1;
                } fields;
                uint32_t raw;
        } stat_payload_t;

        uint32_t format_stat_payload(uint32_t tag, bool kernel, bool end){
                stat_payload_t payload;
                if (kernel && tag) {
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "Kernel %s must have tag value of 0, got %u\n",
                                   me.x, me.y, get_cycle(), __func__, end ? "Finish" : "Start", tag);
                        exit(1);
                }

                if(tag > 15){
                        bsg_pr_err("Tile (X:%d,Y:%d) @ %lu -- %s: "
                                   "%s Tag value greater than 15 not supported, got %u\n",
                                   me.x, me.y, get_cycle(), __func__, end ? "Finish" : "Start", tag);
                        exit(1);
                }

                payload.fields.kernel = kernel;
                payload.fields.end = end;
                payload.fields.y = me.y;
                payload.fields.x = me.x;
                payload.fields.tg_id = tg_id;
                payload.fields.tag = tag;
                return payload.raw;
        }
};

// instantiation of static map.
std::map<BsgDpiTile::key_t, BsgDpiTile*> BsgDpiTile::tiles;

std::string BsgDpiTile::filename = "dpi_stats.csv";
std::ofstream BsgDpiTile::statfile(BsgDpiTile::filename, std::ofstream::out);

// DPI calls expected by Simulator
extern "C"
void bsg_dpi_tile(uint32_t  my_x_i,
                  uint32_t  my_y_i,

                  uint32_t network_req_credits_used_i,
                  bool network_req_v_i,
                  const hb_mc_request_packet_t *network_req_i,

                  bool *endpoint_rsp_v_o,
                  hb_mc_response_packet_t *endpoint_rsp_o,
                  bool endpoint_rsp_ready_i,

                  bool network_rsp_v_i,
                  const hb_mc_response_packet_t *network_rsp_i,

                  bool *endpoint_req_v_o,
                  hb_mc_request_packet_t *endpoint_req_o,
                  bool endpoint_req_ready_i
                  ){

        BsgDpiTile::key_t k = BsgDpiTile::get_key({.x=my_x_i, .y=my_y_i});
        BsgDpiTile *t = BsgDpiTile::get_tile(k);

        t->execute_cycle(network_req_credits_used_i,

                         network_req_v_i,
                         network_req_i,

                         endpoint_rsp_v_o,
                         endpoint_rsp_o,
                         endpoint_rsp_ready_i,

                         network_rsp_v_i,
                         network_rsp_i,

                         endpoint_req_v_o,
                         endpoint_req_o,
                         endpoint_req_ready_i);
}

extern "C"
void bsg_dpi_tile_init(uint32_t num_tiles_y_p,
                       uint32_t num_tiles_x_p,
                       uint32_t noc_coord_width_y_p,
                       uint32_t noc_coord_width_x_p,
                       uint32_t pod_coord_width_y_p,
                       uint32_t pod_coord_width_x_p,
                       
                       uint32_t icache_entries_p,
                       uint32_t dmem_size_p,
                       uint32_t addr_width_p,
                       uint32_t data_width_p,

                       uint32_t max_reg_id,
                       uint32_t credit_counter_width_p,

                       uint32_t vcache_sets_p,
                       uint32_t vcache_block_words_p,
                       uint32_t vcache_stripe_words_p,

                       uint32_t host_x_i,
                       uint32_t host_y_i,

                       uint32_t my_x_i,
                       uint32_t my_y_i){

        // Construct the class for this tile. This will register it in
        // the appropriate x/y location and create DMEM/ICACHE/CSR
        // storage for this instance.
        new BsgDpiTile(num_tiles_y_p,
                       num_tiles_x_p,
                       noc_coord_width_y_p,
                       noc_coord_width_x_p,
                       pod_coord_width_y_p,
                       pod_coord_width_x_p,

                       icache_entries_p,
                       dmem_size_p,
                       addr_width_p,
                       data_width_p,

                       max_reg_id,
                       credit_counter_width_p,

                       vcache_sets_p,
                       vcache_block_words_p,
                       vcache_stripe_words_p,

                       host_x_i,
                       host_y_i,

                       my_x_i,
                       my_y_i);
}


extern "C"
void bsg_dpi_tile_finish(uint32_t my_x_i,
                         uint32_t my_y_i){
        BsgDpiTile::key_t k = BsgDpiTile::get_key({.x=my_x_i, .y=my_y_i});
        BsgDpiTile *t = BsgDpiTile::get_tile(k);

        bsg_pr_dbg("Tile (X:%d,Y:%d) @ %lu -- %s\n",
                   my_x_i, my_y_i, t->get_cycle(), __func__);

        delete t;
}

#endif
