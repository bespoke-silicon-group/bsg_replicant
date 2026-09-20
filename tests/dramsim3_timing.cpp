// Compare the transaction acceptance/completion transcript across DRAM builds.
#include "memory_system.h"
#include <cassert>
#include <cstdint>
#include <iostream>

extern "C" void* bsg_dramsim3_init(int, int, long long, int, char*);
extern "C" void bsg_dramsim3_exit(void*);

int main(int argc, char **argv) {
    if (argc == 3) {
        // Exercise the BSG wrapper's compiled-in configuration directory too.
        char config[] = "HBM2_1Gb_x64_32ba.ini";
        void* memory = bsg_dramsim3_init(1, 256, 1024LL * (1 << 23), 32, config);
        assert(memory);
        bsg_dramsim3_exit(memory);
        return 0;
    }
    assert(argc == 2);
    uint64_t tick = 0, accepted = 0, completed = 0;
    dramsim3::MemorySystem memory(argv[1], ".",
        [&](uint64_t address) {
            std::cout << "R " << tick << " " << address << "\n";
            ++completed;
        },
        [&](uint64_t address) {
            std::cout << "W " << tick << " " << address << "\n";
            ++completed;
        });
    for (; tick < 12000; ++tick) {
        // Start idle so self-refresh can enter before any requests, then wake
        // it with traffic. Include another idle interval between request bursts.
        uint64_t limit = tick < 2000 ? 1024 : 2048;
        if (tick >= 200 && (tick < 2000 || tick >= 4000) && accepted < limit) {
            for (int request = 0; request < 8 && accepted < limit; ++request) {
                uint64_t address = ((accepted * 37) % 8192) * 64;
                bool write = accepted % 3 == 0;
                bool ready = memory.WillAcceptTransaction(address, write);
                std::cout << "A " << tick << " " << accepted << " " << ready << "\n";
                if (!ready) break;
                assert(memory.AddTransaction(address, write));
                ++accepted;
            }
        }
        if (tick == 100) memory.PrintTagStats(0x80000000);
        if (tick == 6000) memory.PrintTagStats(0xc0000000);
        if (tick == 7000) memory.ResetStats();
        memory.ClockTick();
    }
    memory.PrintTagStats(0xc0000001);
    memory.PrintStats();
    assert(accepted == 2048 && completed == accepted);
    std::cout << "PASS " << accepted << " " << completed << "\n";
}
