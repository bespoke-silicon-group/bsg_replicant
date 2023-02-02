
#ifndef _BSG_MANYCORE_SPSC_QUEUE_HPP
#define _BSG_MANYCORE_SPSC_QUEUE_HPP

template <typename T, int S>
class bsg_manycore_spsc_queue_recv {
private:
    hb_mc_device_t *device;
    hb_mc_manycore_t *mc;
    hb_mc_eva_t buffer_eva;
    hb_mc_eva_t count_eva;
    int rptr;

public:
    bsg_manycore_spsc_queue_recv(hb_mc_device_t *device, eva_t buffer_eva, eva_t count_eva)
        : device(device), mc(device->mc), buffer_eva(buffer_eva), count_eva(count_eva), rptr(0) { }

    bool is_empty(void)
    {
        int count;
        void *src = (void *) ((intptr_t) count_eva);
        void *dst = (void *) &count;
        BSG_CUDA_CALL(hb_mc_device_memcpy(device, dst, src, sizeof(T), HB_MC_MEMCPY_TO_HOST));

        return count == 0;
    }

    bool try_recv(T *data)
    {
        if (is_empty())
        {
            return false;
        }

        void *src = (void *) ((intptr_t) buffer_eva+rptr*sizeof(T));
        void *dst = (void *) data;
        BSG_CUDA_CALL(hb_mc_device_memcpy(device, dst, src, sizeof(T), HB_MC_MEMCPY_TO_HOST));
        BSG_CUDA_CALL(hb_mc_manycore_host_request_fence(mc, -1));
        // Probably faster than modulo, but should see if compiler
        //   optimizes...
        if (++rptr == S)
        {
            rptr = 0;
        }
        hb_mc_pod_id_t pod_id = device->default_pod_id;
        hb_mc_pod_t *pod = &device->pods[pod_id];
        hb_mc_npa_t count_npa;
        size_t xfer_sz = sizeof(int);
        BSG_CUDA_CALL(hb_mc_eva_to_npa(mc, &default_map, &pod->mesh->origin, &count_eva, &count_npa, &xfer_sz));
        BSG_CUDA_CALL(hb_mc_manycore_amoadd(mc, &count_npa, -1, NULL));

        return true;
    }
};

template <typename T, int S>
class bsg_manycore_spsc_queue_send {
private:
    hb_mc_device_t *device;
    hb_mc_manycore_t *mc;
    hb_mc_eva_t buffer_eva;
    hb_mc_eva_t count_eva;
    int wptr;

public:
    bsg_manycore_spsc_queue_send(hb_mc_device_t *device, eva_t buffer_eva, eva_t count_eva)
        : device(device), mc(device->mc), buffer_eva(buffer_eva), count_eva(count_eva), wptr(0) { }

    bool is_full(void)
    {
        int count;
        void *src = (void *) ((intptr_t) count_eva);
        void *dst = (void *) &count;
        BSG_CUDA_CALL(hb_mc_device_memcpy(device, dst, src, sizeof(T), HB_MC_MEMCPY_TO_HOST));

        return count == S;
    }

    bool try_send(T data)
    {
        if (is_full())
        {
            return false;
        }

        void *dst = (void *) ((intptr_t) buffer_eva+wptr*sizeof(T));
        void *src = (void *) &data;
        BSG_CUDA_CALL(hb_mc_device_memcpy(device, dst, src, sizeof(T), HB_MC_MEMCPY_TO_DEVICE));
        BSG_CUDA_CALL(hb_mc_manycore_host_request_fence(mc, -1));
        // Probably faster than modulo, but should see if compiler
        //   optimizes...
        if (++wptr == S)
        {
            wptr = 0;
        }
        hb_mc_pod_id_t pod_id = device->default_pod_id;
        hb_mc_pod_t *pod = &device->pods[pod_id];
        hb_mc_npa_t count_npa;
        size_t xfer_sz = sizeof(int);
        BSG_CUDA_CALL(hb_mc_eva_to_npa(mc, &default_map, &pod->mesh->origin, &count_eva, &count_npa, &xfer_sz));
        BSG_CUDA_CALL(hb_mc_manycore_amoadd(mc, &count_npa, 1, NULL));

        return true;
    }
};

#endif

