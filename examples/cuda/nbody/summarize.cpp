#include <bsg_manycore.h>
#include <bsg_manycore_atomic.h>
#include <bsg_cuda_lite_barrier.h>
#include <Octree.hpp>
#include <Body.hpp>

// Compute the center of mass for each node in the tree, using the
// centers of mass of it's children.

// There are several ways to do this. I have implemented a lock-free
// version for the moment. The downside is that this method has
// performance pathologies, where a small number of tiles can end up
// processing a large number of nodes. However, it is simple to write
// and as stated -- lock free.

extern "C" void summarize(HBNode *root, HBBody *bodies, int nBodies, int *idx){

        bsg_barrier_hw_tile_group_init();
        bsg_barrier_hw_tile_group_sync();
        bsg_cuda_print_stat_kernel_start();
        bsg_cuda_print_stat_start(1);
        bsg_fence();

        HBNode *cur;
        HBOctree *pred;

        // The following code is the lock-free version. Tiles start by
        // processing bodies. A body is processed by atomically
        // decrementing the number of children in the parent. When
        // there are no remaining children to be processed, the node
        // that processes the final child processes the parent. If
        // there are remaining children to process, the tile processes
        // another body.

        // Tiles are assigned bodies by atomically incrementing a
        // global index that points to the first unprocessed body.

        int local_idx = __bsg_id;
        unsigned int remaining_children = 0;
        cur = (HBNode *)&bodies[local_idx];
        // While still bodies to process, or not finished processing
        // previous body (local_idx remains unchanged)
        while(local_idx < nBodies){
                pred = (HBOctree *)cur->pred;
                remaining_children = bsg_amoadd(&pred->nChildren, -1);
                // If we are processing the last child, summarize the
                // center of mass of the predecessor.
                if(remaining_children == 1){
                        Point pos;
                        float mass = 0.0f;
                        // This can probably be unrolled to increase the number of requests in flight.
                        for(char octant = 0; octant < HBOctree::octants; octant ++){
                                if(pred->child[octant] != nullptr){
                                        mass += pred->child[octant]->mass;
                                        pos += (pred->child[octant]->pos * pred->child[octant]->mass);
                                }
                        }
                        pred->mass = mass;
                        pred->pos = pos/mass;
                        if(pred == root){
                                break;
                        }
                        else 
                                cur = pred;
                } else {
                        // Otherwise there are children
                        // remaining. Claim a new body.
                        local_idx = bsg_amoadd(idx, 1);
                        cur = (HBNode *)&bodies[local_idx];
                }
        }

        // The following comment is the work queue version. It
        // optimizes for work efficiency. A double-ended work queue
        // primitive is necessary

        // While still nodes to process in work queue
        //   Remove node from work queue
        //   Update center of mass for predecessor:
        //     amofadd(item->pred.mass, item->mass) // We don't have amofadd, just add to lock and read from memory
        //     amofadd(item->pred.pos, item->mass * item->pos) // Not correct. The position should start from 0, not accumulate to the existing position.
        //     rem = amoadd(item.pred->nChildren, -1) // Moving before fadd would improve perf.
        //     if(rem == 0)
        //       item->pred.pos = item->pred.pos / mass
        //       add item to work queue
        
        // The following comment is the "bottom-up" version. It
        // focuses on repeated linear traversal through DRAM, taking
        // advantage of potential DRAM locality.

        // Host: Set start_ptr to &nodes[0]
        // Host: Set end_ptr to &nodes[nbodies-1]
        // Set local_start_ptr = bodies[nbodies-1]
        // Set local_end_ptr = bodies[0]
        // While true
        //   For each node in (start ptr, finish ptr, sizeof(node))
        //     If unprocessed and ready to process
        //       lock predecessor
        //         mass = amofadd(item->pred.mass, item->mass)
        //         pos = amofadd(item->pred.pos, item->mass * item->pos)
        //         rem = amoadd(item.pred->nChildren, -1) // Moving before fadd would improve perf.
        //         if(rem == 0)
        //           item->pred.pos = pos / mass
        //           amomin.u(end_ptr, predecessor ptr)
        //           amomax.u(start_ptr, predecessor ptr)
        //    sync
        //    if local_start_ptr == root
        //      break
        //    fence
        //    local_start_ptr = start_ptr
        //    local_end_ptr = end_ptr
        //    sync
        //    if origin
        //      Set start_ptr to &nodes[0]
        //      Set end_ptr to &nodes[nbodies-1]
        //      fence
        //    sync
        
        bsg_cuda_print_stat_end(1);
        bsg_barrier_hw_tile_group_sync();
        bsg_cuda_print_stat_kernel_end();
        bsg_fence();
        bsg_barrier_hw_tile_group_sync();
        return;
}

