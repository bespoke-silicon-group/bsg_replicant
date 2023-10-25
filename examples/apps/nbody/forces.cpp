#include <cmath>
#include <bsg_manycore_atomic.h>
#include <bsg_manycore.h>
#include <bsg_cuda_lite_barrier.h>

#include <Octree.hpp>
#include <Body.hpp>
#include <Config.hpp>

Point updateForce(Config &cfg, Point delta, float psq, float mass) {
        // Computing force += delta * mass * (|delta|^2 + eps^2)^{-3/2}
        float idr   = 1.0f / sqrtf((float)(psq + cfg.epssq));
        float scale = mass * idr * idr * idr;
        return delta * scale;
}

extern "C" void forces(Config *pcfg, HBOctree *proot, HBBody *HBBodies, int nBodies, unsigned int _diam, int *amocur, int body_end){
        // We can't pass float arguments (technical issue), just
        // pointers and integer arguments.
        // Copy frequently used data to local
        Config cfg = *pcfg;
        float diam = *reinterpret_cast<float *>(&_diam);
        bsg_barrier_hw_tile_group_init();
        bsg_barrier_hw_tile_group_sync();   
        bsg_cuda_print_stat_kernel_start();
        bsg_cuda_print_stat_start(1);
        // In the x86 version, threads use a stack to do a DFS
        // traversal of a tree. We can't do that here because we have
        // limited stack space. Instead, we are going to do an
        // in-order DFS traversal. Ordering is ensured by storing the
        // parent's octant index in the child. When we move up the
        // tree we use the octant information in the child to select
        // the next octant in the parent node.
        
        // dsq = cfg.dsq
        // While still bodies to process
        //   Remove body b from work queue, copy to wb
        //   set cur to root
        //   octant = 0
        //   while cur != root, octant < 8
        //     if (cur.isLeaf)
        //       curd   = L1 distance between wb, cur
        //       curdsq = L2 distance between wb, cur
        //       b.acc += updateForce(curd, curdsq, b.mass)
        //       octant = cur.octant + 1
        //       cur = cur.pred
        //       diamsq * 4.0f        
        //     else if (octant == 0) -- octant of 0 means first time we have visited this node
        //       curd   = L1 distance between wb, cur
        //       curdsq = L2 distance between wb, cur
        //       if curdsq >= dsq
        //         b.acc += updateForce(curd, curdsq, b.mass)
        //         octant = cur.octant + 1
        //         cur = cur.pred
        //         diamsq * 4.0f
        //     else if (octant < 8)
        //        if cur.child[octant] != NULL
        //          cur = cur.child[octant]
        //          diamsq *= .25f
        //        else
        //          octant ++
        //     else
        //       octant = cur.octant + 1
        //       cur = cur.pred
        //       diamsq * 4.0f

        // Work imbalance is a pain. Use an amoadd queue to allocate work dynamically.
        int cur;
        cur = bsg_amoadd(amocur, 1);
        while(cur < body_end){
                //	for (int cur = __bsg_id + body_start; cur < body_end; cur += TILE_GROUP_DIM_X * TILE_GROUP_DIM_Y) {
                bsg_print_int(cur);
                float diamsq = diam * diam * cfg.itolsq;
                HBBody *pcurb = &HBBodies[cur];
                HBBody curb = *pcurb;
                Point prev_acc = curb.acc;
                curb.acc = Point(0.0f, 0.0f, 0.0f);
                HBNode *pother = proot; // It is not clear whether copying the other node locally will help.
                Point delta;
                float distsq;
                char octant = 0;
                while (pother != proot || (pother == proot && octant < HBOctree::octants)){
                        // Leaf status is encoded in the low-order bit of the address.
                        unsigned int leaf = reinterpret_cast<uintptr_t>(pother) & 1;
                        pother = reinterpret_cast<HBNode*>(reinterpret_cast<uintptr_t>(pother) & (~1));
                        HBOctree *node = static_cast<HBOctree *>(pother);

                        // child && !(((unsigned int) child) & 1)
                        if(leaf){
                                //bsg_print_float(1.0f);
                                // Leaf node, compute force
                                if(pother != static_cast<HBNode *>(pcurb)){
                                        //bsg_print_float(1.1f);
                                        delta  = (curb.pos - pother->pos);
                                        distsq = delta.dist2();
                                        curb.acc += updateForce(cfg, delta, distsq, pother->mass);
                                }
                                // Move upward
                                octant = pother->octant + 1;
                                pother = pother->pred;
                                diamsq *= 4.0f;
                                //bsg_print_hexadecimal((unsigned int)pother);
                        } else if (octant == 0) {
                                //bsg_print_float(2.0f);
                                //bsg_print_hexadecimal((unsigned int)node->child[0]);
                                // Have not visited this node before
                                delta  = (curb.pos - pother->pos);
                                distsq = delta.dist2();
                                if(distsq >= diamsq){
                                        //bsg_print_float(2.1f);
                                        // Far away, compute summarized force
                                        curb.acc += updateForce(cfg, delta, distsq, pother->mass);
                                        // Move upward
                                        octant = pother->octant + 1;
                                        pother = pother->pred;
                                        diamsq *= 4.0f;
                                        //bsg_print_hexadecimal((unsigned int)pother);
                                } else if(node->child[0] != nullptr){
                                        //bsg_print_float(2.2f);
                                        // Nearby, downward.
                                        pother = node->child[0];
                                        octant = 0;
                                        diamsq *= .25f;
                                        //bsg_print_hexadecimal((unsigned int)pother);
                                } else {
                                        //bsg_print_float(2.3f);
                                        // No leaf/body, try next octant
                                        octant ++;
                                }
                        } else if (octant < HBOctree::octants) {
                                //bsg_print_float(3.0f);
                                if(node->child[octant]) {
                                        //bsg_print_float(3.1f);
                                        // Downward
                                        pother = node->child[octant];
                                        octant = 0;
                                        diamsq *= .25f;
                                        //bsg_print_hexadecimal((unsigned int)pother);
                                } else {
                                        //bsg_print_float(3.2f);
                                        // Sideways :p 
                                        octant ++;
                                }
                        } else {
                                //bsg_print_float(4.0f);                                
                                // Upward
                                octant  = pother->octant + 1;
                                pother  = pother->pred;
                                diamsq *= 4.0f;
                                //bsg_print_hexadecimal((unsigned int)pother);
                        }
                }
                // Finished traversal
                curb.vel += (curb.acc - prev_acc) * cfg.dthf;
                HBBodies[cur].vel = curb.vel;
                HBBodies[cur].acc = curb.acc;
                cur = bsg_amoadd(amocur, 1);                
        }

        bsg_cuda_print_stat_end(1);
        bsg_barrier_hw_tile_group_sync();
        bsg_cuda_print_stat_kernel_end();
        bsg_fence();
        bsg_barrier_hw_tile_group_sync();

        return;
}
