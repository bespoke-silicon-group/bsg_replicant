//This kernel performs a barrier among all tiles in tile group 

#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"
#include "bsg_manycore_spsc_queue.hpp"

#define BUFFER_ELS  10
#define CHAIN_LEN    4
#define NUM_PACKETS 100

extern "C" __attribute__ ((noinline))
int kernel_host_stream(int *buffer_chain, int *buffer_count)
{
    int *recv_buffer = &buffer_chain[0] + (__bsg_id * BUFFER_ELS);
    int *recv_count  = &buffer_count[0] + (__bsg_id);

    int *send_buffer = &buffer_chain[0] + ((__bsg_id+1) * BUFFER_ELS);
    int *send_count = &buffer_count[0] + (__bsg_id+1);

    bsg_manycore_spsc_queue_recv<int, BUFFER_ELS> recv_spsc(recv_buffer, recv_count);
    bsg_manycore_spsc_queue_send<int, BUFFER_ELS> send_spsc(send_buffer, send_count);

    int data;
    while(1)
    {
        data = recv_spsc.recv();
        send_spsc.send(data);
    }

	return 0;
}

