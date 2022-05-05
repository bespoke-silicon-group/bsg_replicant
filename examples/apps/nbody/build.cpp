
#define BSG_TILE_GROUP_X_DIM bsg_tiles_X
#define BSG_TILE_GROUP_Y_DIM bsg_tiles_Y
#include <bsg_mcs_mutex.hpp>
#include <bsg_manycore.h>
#include <bsg_manycore_atomic.h>
#include <bsg_cuda_lite_barrier.h>
#include <Octree.hpp>
#include <Body.hpp>
#include <atomic>
#include <Config.hpp>

// Given a list of bodies, build an octree



// nNodes - How many nodes have been pre-allocated (Should be at least
// nBodies * log2(nBodies) and more than 128). Not currently used, but
// tiles could check that they haven't gone beyond the end of the
// array.

// This kernel hasn't been completely tested, or optimized. It's not
// clear that all the fences are necessary, for example. In some cases
// I want the compiler to insert memory fences, not actual fence
// instructions

extern "C" void build(Config *pcfg, HBOctree *nodes, int nNodes, int *nidx, HBBody *bodies, int nBodies, int *bidx, unsigned int _radius){

        Config cfg = *pcfg;
        HBOctreeTraverser<3> t;
        HBOctree *cur;
        HBNode *child;
        HBBody *ins;
        // To avoid hammering the nidx "allocator" and bidx "work
        // queue" at the start we pre-allocate every tile a new node,
        // and every tile a body.
        //
        // NOTE: This does mean that nNodes > 128
        // We add one because the root is already created
        int local_nidx = __bsg_id + 1, local_bidx = __bsg_id, depth;
        HBOctree *newNode = &nodes[local_nidx];
        unsigned int remaining_children = 0;
        unsigned int octant;

        bsg_barrier_hw_tile_group_init();
        bsg_barrier_hw_tile_group_sync();
        bsg_cuda_print_stat_kernel_start();
        bsg_cuda_print_stat_start(1);
        bsg_fence();

        // TODO: The biggest remaining challenge is to maximize the
        // load-use distance of the Point position variables
        //
        // Using bsg_attr_remote is probably the right way forward,
        // but C++/LLVM has a lot of issues resolving cross-address
        // space assignments.
        //
        // For the moment, I have tried to separate loading the Point
        // triple from using it. It doesn't work perfectly, but it's
        // better than nothing.

        // While bodies remain in the "work queue"
        while(local_bidx < nBodies){
                //bsg_print_hexadecimal(0x01000000 | local_bidx);
                float radius = *reinterpret_cast<float *>(&_radius);
                ins = &bodies[local_bidx]; // Find a new node to insert
                Point ins_pos = ins->pos;
                Point cur_pos;
                cur = nodes; // Start at root
                cur_pos = cur->pos;

                // Get the octant relative to the current node
                octant = cur_pos.getChildIndex(ins_pos);
                child = cur->child[octant];

                // We may use the depth to switch between a spin lock
                // and a MCS lock in the future. An MCS lock is great
                // for high contention but has higher overhead. A spin
                // lock is great for low contention but can cause
                // network congestion.
                depth = 0;

                // "Recurse" until a null pointer or a leaf is found.
                recurse:
                while(child && !(((unsigned int) child) & 1)){
                        cur = (HBOctree *)child;
                        cur_pos = cur->pos;
                        octant = cur_pos.getChildIndex(ins_pos);
                        //bsg_print_hexadecimal(0x02000000 | octant);
                        child = cur->child[octant];
                        //bsg_print_hexadecimal((unsigned int) child);
                        depth++;
                        // This radius update does not match the CPU code but it does seem to match the GPU code.
                        // The compiler tries to schedule the access child = cur->child[octant] after the radius computation.
                        // These force the compiler to do otherwise.
                        asm volatile("" ::: "memory");                        
                        radius *= 0.5f;
                        asm volatile("" ::: "memory");
                }

                // We have reached the point where we need to modify a
                // node. Lock it.
                //bsg_print_hexadecimal(0x10000000 | local_bidx);
                t.lock(depth, cur);


                // Make sure that the compiler doesn't use stale
                // values from cur->child and child->Leaf. This can
                // happen when another tile grabs the lock before us
                // and updates cur->child[octant].
                std::atomic_thread_fence(std::memory_order_acquire);
                child = cur->child[octant];
                // DR: These next few operations could be squashed,
                // but we need to increase the load-use distance
                // between child and checking if it is a nullptr.
                ins->pred = cur;
                ins->octant = octant;

                // A null pointer means we can directly insert the current body as a leaf.
                if(cur->child[octant] == nullptr){
                        //bsg_print_hexadecimal(0x11000000);

                        bsg_amoadd(&(cur->nChildren), 1);
                        
                        // There is a race condition here. A running
                        // tile can see that cur->child is not null
                        // and assume that the lock is not taken. I
                        // believe this is OK though, since all tiles
                        // waiting on the lock will see any subsequent
                        // updates made by the winning tile. The cost
                        // of removing this race condition is a lot
                        // more locking.
                        // DR: The low-order bit of the address is a leaf flag.
                        cur->child[octant] = (HBBody *)(((unsigned int) ins) | 1);

                        // Fence before unlocking? Max's lock might handle this.
                        // This should probably be before insertion.
                        std::atomic_thread_fence(std::memory_order_release);
                        // Update our local index for inserting a new node
                        local_bidx = bsg_amoadd(bidx, 1);
                        t.unlock(depth, cur);
                        // We've inserted a new node. Start external while loop again.
                        continue;
                }

                // Leaf/body means we need to create a new node and
                // insert both bodies
                // DR: The low-order bit of the pchild address indicates it is a leaf.
                if(((unsigned int) child) & 1){
                        //bsg_print_hexadecimal(0x12000000 | local_nidx);
                        //bsg_print_float(radius);
                        //bsg_print_float((inps->pos - child->pos).dist2());
                        HBBody * leaf = (HBBody *) (((unsigned int)child) & (~1));
                        Point leaf_pos = leaf->pos;

                        if(radius == 0.0f){
                                bsg_print_hexadecimal(0xe0000000 | local_bidx);
                                local_bidx = bsg_amoadd(bidx, 1);
                                t.unlock(depth, cur);
                                continue;
                        }
                        
                        // Octree* new_node = &T.emplace(updateCenter(node->pos, index, radius));
                        // We are inserting a new node below this one, so pass it it's radius.
                        newNode->pos = updateCenter(cur_pos, octant, radius * .5f);

                        // Now that we used local_nidx, get a new one.
                        local_nidx = bsg_amoadd(nidx, 1);

                        // Insert original child
                        char new_octant = newNode->pos.getChildIndex(leaf_pos);
                        // Child already had the LOB set for leaf
                        newNode->child[new_octant] = child;

                        // DR: If the position of the former child,
                        // and the current child are identical then
                        // add some jitter to guarantee uniqueness.
                        // Usually we don't use == for floats, but it
                        // is important in this case.
                        if(ins_pos == leaf_pos){
                                bsg_print_hexadecimal(0xf0000000 | local_bidx);                                
                                float jitter = cfg.tol * .5f;
                                // assert(jitter < radius);
                                ins->pos += (newNode->pos - ins_pos) * jitter;
                        }

                        // Fence after setting children, so that when
                        // we set cur->child other tiles won't see
                        // outdated state.
                        std::atomic_thread_fence(std::memory_order_release);

                        // Set child of cur to newNode. There is a
                        // race condition here: A running tile can see
                        // that cur->child is not null and assume that
                        // the lock is not taken. I believe this is OK
                        // though, because the state of the child is
                        // consistent (because we fenced, above)
                        // before we issued this write.
                        cur->child[octant] = newNode;

                        // Now we just need to fence before unlocking
                        // so that tiles on the lock won't see
                        // inconsistent state of cur->child[octant].
                        std::atomic_thread_fence(std::memory_order_release);

                        // We have inserted a new node at the former
                        // location of child. Set child for the next
                        // iteration of the recursion loop.
                        child = newNode;

                        // We are finished updating newNode, so we can
                        // get a new pointer. This also maximizes
                        // load-use distance.
                        newNode = &nodes[local_nidx];
                }

                // If both conditions above fail, another tile managed
                // to insert while we waited for a lock.

                // If we entered child->Leaf, then we created a new
                // node, and inserted the previous cur->child[octant]
                // into that node. We should try inserting our current
                // body again.

                // Either outcome, the result is the same. Try inserting again.

                // Unlock
                //bsg_print_hexadecimal(0x14000000);
                t.unlock(depth, cur);
                // Retry inserting the existing body.
                goto recurse;
        }

        bsg_cuda_print_stat_end(1);
        bsg_barrier_hw_tile_group_sync();
        bsg_cuda_print_stat_kernel_end();
        bsg_fence();
        bsg_barrier_hw_tile_group_sync();
        return;
}

