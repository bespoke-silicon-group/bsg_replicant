// Host-only checks using a generated machine ROM; no simulator is launched.
#include <bsg_manycore.h>
#include <bsg_manycore_config_pod.h>
#include <bsg_manycore_eva.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <tuple>

int main(int argc, char **argv) {
    assert(argc == 2);
    std::ifstream input(argv[1]);
    assert(input);
    hb_mc_config_raw_t raw[HB_MC_CONFIG_MAX] = {};
    std::string line;
    for (unsigned i = 0; i < HB_MC_CONFIG_MAX; ++i) {
        assert(std::getline(input, line));
        raw[i] = std::stoul(line, nullptr, 2);
    }
    hb_mc_manycore_t mc = {};
    mc.dram_enabled = 1;
    assert(hb_mc_config_init(raw, &mc.config) == HB_MC_SUCCESS);
    const auto &cfg = mc.config;
    const auto pod = hb_mc_coordinate(0, 0);
    const auto origin = hb_mc_config_pod_vcore_origin(&cfg, pod);
    assert(origin.x == cfg.origin.x && origin.y == cfg.origin.y);
    std::set<std::pair<unsigned, unsigned>> cores, caches;
    hb_mc_coordinate_t c;
    hb_mc_config_pod_foreach_vcore(c, pod, &cfg) {
        assert(hb_mc_config_is_vanilla_core(&cfg, c));
        assert(!hb_mc_config_is_dram(&cfg, c));
        cores.emplace(c.x, c.y);
    }
    hb_mc_config_pod_foreach_dram(c, pod, &cfg) {
        assert(hb_mc_config_is_dram(&cfg, c));
        assert(!hb_mc_config_is_vanilla_core(&cfg, c));
        auto id = hb_mc_config_pod_dram_id(&cfg, c);
        auto inverse = hb_mc_config_pod_dram(&cfg, pod, id);
        assert(inverse.x == c.x && inverse.y == c.y);
        caches.emplace(c.x, c.y);
    }
    assert(cores.size() == cfg.pod_shape.x * cfg.pod_shape.y);
    assert(caches.size() == 2 * cfg.pod_shape.x);
    std::set<std::tuple<unsigned, unsigned, unsigned>> addresses;
    uint64_t fingerprint = 14695981039346656037ull;
    // Cross every stripe, cache bank and many cache-capacity boundaries.
    for (unsigned offset = 0; offset < 1048576; offset += 4) {
        hb_mc_eva_t eva = 0x80000000u + offset, inverse;
        hb_mc_npa_t npa;
        size_t bytes;
        assert(default_eva_to_npa(&mc, &origin, &origin, &eva, &npa, &bytes) == HB_MC_SUCCESS);
        assert(caches.count({hb_mc_npa_get_x(&npa), hb_mc_npa_get_y(&npa)}));
        assert(addresses.emplace(hb_mc_npa_get_x(&npa), hb_mc_npa_get_y(&npa), hb_mc_npa_get_epa(&npa)).second);
        for (auto word : {hb_mc_npa_get_x(&npa), hb_mc_npa_get_y(&npa), hb_mc_npa_get_epa(&npa)}) {
            fingerprint ^= word;
            fingerprint *= 1099511628211ull;
        }
        // The existing reverse mapper does not invert iPoly. These tiny profiles disable it.
        if (!cfg.vcache_ipoly_hashing) {
        assert(default_npa_to_eva(&mc, &origin, &origin, &npa, &inverse, &bytes) == HB_MC_SUCCESS);
        assert(inverse == eva);
        }
    }
    std::cout << cfg.pod_shape.x << "x" << cfg.pod_shape.y << ": "
              << cores.size() << " cores, " << caches.size()
              << " caches; 262144 unique word addresses; round trips "
              << (cfg.vcache_ipoly_hashing ? "skipped (iPoly)" : "passed")
              << "; mapping fingerprint " << fingerprint << "\n";
}
