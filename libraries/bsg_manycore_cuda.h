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

#ifndef BSG_MANYCORE_CUDA_H
#define BSG_MANYCORE_CUDA_H
#include <bsg_manycore_features.h>
#include <bsg_manycore_eva.h>

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


        // Kernel is not loaded into tile if kernel poitner equals this value.
#define HB_MC_CUDA_KERNEL_NOT_LOADED_VAL        0x0001
        // The value that is written on to finish_signal_addr to show that tile group execution is done.
#define HB_MC_CUDA_FINISH_SIGNAL_VAL            0xFACE
        // The begining of section in host memory intended for tile groups to write finish signals into.
#define HB_MC_CUDA_HOST_FINISH_SIGNAL_BASE_ADDR 0xF000  



        typedef uint8_t tile_group_id_t;
        typedef uint8_t grid_id_t;
        typedef int hb_mc_allocator_id_t;


        typedef enum {
                HB_MC_TILE_GROUP_STATUS_INITIALIZED=0,
                HB_MC_TILE_GROUP_STATUS_ALLOCATED=1,
                HB_MC_TILE_GROUP_STATUS_LAUNCHED=2,
                HB_MC_TILE_GROUP_STATUS_FINISHED=3,
        } hb_mc_tile_group_status_t ;


        typedef enum {
                HB_MC_TILE_STATUS_FREE=0,
                HB_MC_TILE_STATUS_BUSY=1,
        } hb_mc_tile_status_t;


        typedef struct {
                hb_mc_coordinate_t coord;
                hb_mc_coordinate_t origin;      
                hb_mc_coordinate_t tile_group_id;
                hb_mc_tile_status_t status;
        } hb_mc_tile_t;

        typedef struct {
                const char     *name;
                uint32_t        argc;
                const uint32_t *argv;
                int             refcount;
        } hb_mc_kernel_t;

        typedef struct {
                hb_mc_coordinate_t        id;
                grid_id_t                 grid_id;
                hb_mc_dimension_t         grid_dim;
                hb_mc_tile_group_status_t status;
                hb_mc_coordinate_t        origin;
                hb_mc_dimension_t         dim;
                hb_mc_eva_map_t          *map;
                hb_mc_kernel_t           *kernel;
                hb_mc_eva_t               argv_eva;
                hb_mc_eva_t               barcfg_eva;
                hb_mc_npa_t               finish_signal_npa;
        } hb_mc_tile_group_t;


        typedef struct {
                hb_mc_dimension_t dim;
                hb_mc_coordinate_t origin;
                hb_mc_tile_t* tiles;
        } hb_mc_mesh_t;


        typedef struct {
                hb_mc_allocator_id_t id;
                const char *name; 
                void *memory_manager;
        } hb_mc_allocator_t;


        typedef struct hb_mc_program_options {
                hb_mc_allocator_id_t alloc_id;
                const char          *alloc_name;
                hb_mc_dimension_t    mesh_dim;
                const char          *program_name;
                // by default CUDA will 'copy' program data into an internal buffer
                // set this option to 1 if CUDA should instead take ownership of the data passed
                // this is only applicable to hb_mc_device_pod_program_init_binary_*()
                int                  move_bin_data;
        } hb_mc_program_options_t;

        typedef struct {
                const char* bin_name;
                const unsigned char* bin;
                size_t bin_size;
                hb_mc_allocator_t *allocator;
        } hb_mc_program_t;

        typedef int hb_mc_pod_id_t;

        typedef struct {
                hb_mc_program_t    *program;
                hb_mc_mesh_t       *mesh;
                hb_mc_tile_group_t *tile_groups;
                uint32_t            num_tile_groups;
                uint32_t            tile_group_capacity;
                uint8_t             num_grids;
                hb_mc_coordinate_t  pod_coord; // what pod am I in the global manycore?
                int                 program_loaded;
        } hb_mc_pod_t;

        typedef struct {
                hb_mc_manycore_t *mc;
                hb_mc_pod_t      *pods;
                hb_mc_pod_id_t    num_pods;
                const char       *name;
                hb_mc_pod_id_t    default_pod_id;
                hb_mc_dimension_t default_mesh_dim;
        } hb_mc_device_t; 


        enum hb_mc_memcpy_kind {
                HB_MC_MEMCPY_TO_DEVICE = 0,
                HB_MC_MEMCPY_TO_HOST = 1,
        };

        void hb_mc_program_options_default(hb_mc_program_options_t *popts);

        static inline hb_mc_pod_id_t hb_mc_device_pods(hb_mc_device_t *device)
        {
                return device->num_pods;
        }

        static inline int hb_mc_device_set_default_pod(hb_mc_device_t *device,
                                                       hb_mc_pod_id_t  pod)
        {
                if (pod < device->num_pods) {
                        device->default_pod_id = pod;
                        return HB_MC_SUCCESS;
                } else {
                        return HB_MC_INVALID;
                }
        }

#define hb_mc_device_foreach_pod_id(device_ptr, pod_id)   \
        for (pod_id = 0; pod_id < (device_ptr)->num_pods; pod_id++)

        /********************************/
        /* Pod Interface Initialization */
        /********************************/
        /**
         * Initializes a CUDA-Lite program on the manycore on a pod specified.
         * @param[in] device   Pointer to device
         * @param[in] pod      Pod ID
         * @param[in] bin_name Path to program file
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_pod_program_init(hb_mc_device_t *device,
                                          hb_mc_pod_id_t  pod,
                                          const char     *bin_name);

        /**
         * Initializes a CUDA-Lite program on the manycore on a pod specified.
         * @param[in] device   Pointer to device
         * @param[in] pod      Pod ID
         * @param[in] bin_name Path to program file
         * @param[in] popts    Program options defining program behavior
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_pod_program_init_opts(hb_mc_device_t *device,
                                               hb_mc_pod_id_t  pod,
                                               const char     *bin_name,
                                               const hb_mc_program_options_t *popts);


        /**
         * Initializes a CUDA-Lite program on the manycore on a pod specified.
         * @param[in] device   Pointer to device
         * @param[in] pod      Pod ID
         * @param[in] bin_data Buffer with program data
         * @param[in] bin_size Size of program data buffer
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_pod_program_init_binary(hb_mc_device_t       *device,
                                                 hb_mc_pod_id_t        pod_id,
                                                 const unsigned char  *bin_data,
                                                 size_t                bin_size);


       /**
        * Initializes a CUDA-Lite program on the manycore on a pod specified.
         * @param[in] device   Pointer to device
         * @param[in] pod      Pod ID
         * @param[in] bin_data Buffer with program data
         * @param[in] bin_size Size of program data buffer
         * @param[in] popts    Program options defining program behavior
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_pod_program_init_binary_opts(hb_mc_device_t       *device,
                                                      hb_mc_pod_id_t        pod_id,
                                                      const unsigned char  *bin_data,
                                                      size_t                bin_size,
                                                      const hb_mc_program_options_t *popts);
        /****************************/
        /* Pod Interface Allocation */
        /****************************/
        /**
         * Allocates memory on device's DRAM associated with the input pod
         * hb_mc_device_pod_program_init() should have been called for device and pod
         * before calling this function to set up a memory allocator.
         * @param[in]  device        Pointer to device
         * @param[in]  pod           Pod ID with a prorgam initialized
         * @parma[in]  size          Size of requested memory
         * @param[out] eva           Eva address of the allocated memory
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_pod_malloc(hb_mc_device_t *device,
                                    hb_mc_pod_id_t  pod,
                                    uint32_t        size,
                                    hb_mc_eva_t    *eva);

        /**
         * Frees memory on device's DRAM associated with the input pod
         * hb_mc_device_pod_program_init() should have been called for device and pod
         * before calling this function to set up a memory allocator.
         * @param[in]  device        Pointer to device
         * @param[in]  pod           Pod ID with a prorgam initialized
         * @param[out] eva           Eva address of the memory to be freed
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_pod_free(hb_mc_device_t *device,
                                  hb_mc_pod_id_t  pod,
                                  hb_mc_eva_t     eva);

        /*******************************/
        /* Pod Interface Data Movement */
        /*******************************/
        /**
         * Copies a buffer from src on the host/pod DRAM to dst on pod DRAM/host.
         * @param[in]  device        Pointer to device
         * @param[in]  pod           Pod ID
         * @parma[in]  src           EVA address of source to be copied from
         * @parma[in]  dst           EVA address of destination to be copied into
         * @param[in]  name          EVA address of dst
         * @param[in]  count         Size of buffer (number of bytes) to be copied
         * @param[in]  hb_mc_memcpy_kind         Direction of copy (HB_MC_MEMCPY_TO_DEVICE / HB_MC_MEMCPY_TO_HOST)
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_pod_memcpy(hb_mc_device_t *device,
                                    hb_mc_pod_id_t pod,
                                    void *dst,
                                    const void *src,
                                    uint32_t bytes,
                                    enum hb_mc_memcpy_kind kind);

        /**
         * Copies a buffer from src on the host to dst on pod DRAM.
         * @param[in]  device        Pointer to device
         * @param[in]  pod           Pod ID
         * @parma[in]  daddr         EVA address of destination to be copied into
         * @parma[in]  haddr         Host address of source to be copied from
         * @param[in]  bytes         Size of buffer to be copied
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_pod_memcpy_to_device(hb_mc_device_t *device,
                                              hb_mc_pod_id_t pod,
                                              hb_mc_eva_t daddr,
                                              const void *haddr,
                                              uint32_t bytes);

        /**
         * Copies a buffer from src on pod DRAM to dst on host.
         * @param[in]  device        Pointer to device
         * @param[in]  pod           Pod ID
         * @parma[in]  daddr         EVA address of destination to be copied into
         * @parma[in]  haddr         Host address of source to be copied from
         * @param[in]  bytes         Size of buffer to be copied
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_pod_memcpy_to_host(hb_mc_device_t *device,
                                            hb_mc_pod_id_t pod,
                                            void *haddr,
                                            hb_mc_eva_t daddr,
                                            uint32_t bytes);

        /**
         * Sets memory to a given value starting from an address in pod's DRAM.
         * @param[in]  device        Pointer to device
         * @param[in]  pod           Pod ID
         * @parma[in]  eva           EVA address of destination
         * @param[in]  val           Value to be written out
         * @param[in]  sz            The number of bytes to write into device DRAM
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_pod_memset (hb_mc_device_t *device,
                                     hb_mc_pod_id_t pod,
                                     hb_mc_eva_t  eva,
                                     uint8_t data,
                                     size_t sz);

        /***********************************/
        /* Pod Interface Execution Control */
        /***********************************/
        /**
         * Enqueues and schedules a kernel to be run on a pod
         * Takes the grid size, tile group dimensions, kernel name, argc,
         * argv* and adds a kernel invocation to a queue.
         *
         * When the kernel is invoked, it will execute with tile groups of size
         * given by tg_dim. The number of tile groups is given by the dimension
         * specified in grid_dim.
         *
         * The kernels are not launched with this call.
         * See hb_mc_device_pod_kernels_execute() to launch enqueued kernels.
         * @param[in]  device        Pointer to device
         * @param[in]  pod           Pod ID
         * @param[in]  grid_dim      X/Y dimensions of the grid to be initialized
         * @param[in]  tg_dim        X/Y dimensions of tile groups in grid
         * @param[in]  name          Kernel name to be executed on tile groups in grid
         * @param[in]  argc          Number of input arguments to kernel
         * @param[in]  argv          List of input arguments to kernel
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_pod_kernel_enqueue(hb_mc_device_t *device,
                                            hb_mc_pod_id_t  pod,
                                            hb_mc_dimension_t grid_dim,
                                            hb_mc_dimension_t tg_dim,
                                            const char *name,
                                            const uint32_t argc,
                                            const uint32_t *argv);

        /**
         * Launches all kernel invocations enqueued on pod.
         * These kernel invocations are enqueued by
         * hb_mc_device_pod_kernel_enqueue().
         *
         * This function blocks until all kernels have been invoked
         * and completed.
         * @param[in]  device        Pointer to device
         * @param[in]  pod           Pod ID
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_pod_kernels_execute(hb_mc_device_t *device,
                                             hb_mc_pod_id_t pod);

        /**
         * Launches all kernel invocations enqueued on pods.
         * These kernel invocations are enqueued by
         * hb_mc_device_pod_kernel_enqueue().
         *
         * This function blocks until all kernels have been invoked
         * and completed.
         * @param[in]  device        Pointer to device
         * @param[in]  podv          Vector of Pod IDs
         * @param[in]  podc          Number of Pod IDs
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_podv_kernels_execute(hb_mc_device_t *device,
                                              hb_mc_pod_id_t *podv,
                                              int podc);

        /**
         * Launches all kernel invocations enqueued on all pods.
         * These kernel invocations are enqueued by
         * hb_mc_device_pod_kernel_enqueue().
         *
         * This function blocks until all kernels have been invoked
         * and completed.
         * @param[in]  device        Pointer to device
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_pods_kernels_execute(hb_mc_device_t *device);

        /*************************/
        /* Pod Interface Cleanup */
        /*************************/
        /**
         * Performs cleanup for a program loaded onto pod with
         * hb_mc_device_pod_program_init().
         *
         * All memory allocated with hb_mc_device_pod_malloc() is
         * freed.
         *
         * After this function is called, a new program can be
         * initialized on pod using hb_mc_device_pod_program_init().
         * @param[in]  device        Pointer to device
         * @param[in]  pod           Pod ID
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_pod_program_finish(hb_mc_device_t *device,
                                            hb_mc_pod_id_t pod);

        /********************/
        /* Legacy Interface */
        /********************/
        /**
         * Initializes the manycor struct, and a mesh structure with default (maximum)
         * diemsnions inside device struct with list of all tiles and their cooridnates 
         * @param[in]  device        Pointer to device
         * @param[in]  name          Device name
         * @param[in]  id            Device id
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_init (hb_mc_device_t *device,
                               const char *name,
                               hb_mc_manycore_id_t id);





        /**
         * Initializes the manycor struct, and a mesh structure with custom
         * diemsnions inside device struct with list of all tiles and their cooridnates 
         * @param[in]  device        Pointer to device
         * @param[in]  name          Device name
         * @param[in]  id            Device id
         * @param[in]  dim           Tile pool (mesh) dimensions
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result, deprecated))
        int hb_mc_device_init_custom_dimensions (hb_mc_device_t *device,
                                                 const char *name,
                                                 hb_mc_manycore_id_t id,
                                                 hb_mc_dimension_t dim);




        /**
         * Takes in a buffer containing the binary and its size,
         * freezes tiles, loads program binary into all tiles and into dram,
         * and sets the symbols and registers for each tile.
         * @param[in]  device        Pointer to device
         * @parma[in]  bin_name      Name of binary elf file
         * @param[in]  bin_data      Buffer containing binary 
         * @param[in]  bin_size      Size of the binary to be loaded onto device
         * @param[in]  id            Id of program's memory allocator
         * @param[in]  alloc_name    Unique name of program's memory allocator
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_program_init_binary (hb_mc_device_t *device,
                                              const char *bin_name,
                                              const unsigned char *bin_data,
                                              size_t bin_size,
                                              const char *alloc_name,
                                              hb_mc_allocator_id_t id);





        /**
         * Takes in a binary name, loads the binary from file onto a buffer,
         * freezes tiles, loads program binary into all tiles and into dram,
         * and sets the symbols and registers for each tile.
         * @param[in]  device        Pointer to device
         * @parma[in]  bin_name      Name of binary elf file
         * @param[in]  id            Id of program's memory allocator
         * @param[in]  alloc_name    Unique name of program's memory allocator
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_program_init (hb_mc_device_t *device,
                                       const char *bin_name,
                                       const char *alloc_name,
                                       hb_mc_allocator_id_t id);



        /**
         * Allocates memory on device DRAM
         * hb_mc_device_program_init() or hb_mc_device_program_init_binary() should
         * have been called before calling this function to set up a memory allocator.
         * @param[in]  device        Pointer to device
         * @parma[in]  size          Size of requested memory
         * @param[out] eva           Eva address of the allocated memory
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_malloc (hb_mc_device_t *device, uint32_t size, hb_mc_eva_t *eva);





        /**
         * Copies a buffer from src on the host/device DRAM to dst on device DRAM/host.
         * @param[in]  device        Pointer to device
         * @parma[in]  src           EVA address of source to be copied from
         * @parma[in]  dst           EVA address of destination to be copied into
         * @param[in]  name          EVA address of dst
         * @param[in]  count         Size of buffer (number of bytes) to be copied
         * @param[in]  hb_mc_memcpy_kind         Direction of copy (HB_MC_MEMCPY_TO_DEVICE / HB_MC_MEMCPY_TO_HOST)
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_memcpy (hb_mc_device_t *device,
                                 void *dst,
                                 const void *src,
                                 uint32_t count,
                                 enum hb_mc_memcpy_kind kind);


        /**
         * Copies a buffer from src on the host to dst on device DRAM.
         * @param[in]  device        Pointer to device
         * @parma[in]  daddr         EVA address of destination to be copied into
         * @parma[in]  haddr         Host address of source to be copied from
         * @param[in]  bytes         Size of buffer to be copied
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_memcpy_to_device(hb_mc_device_t *device,
                                          hb_mc_eva_t daddr,
                                          const void *haddr,
                                          uint32_t bytes);

        /**
         * Copies a buffer from src on device DRAM to dst on device the host.
         * @param[in]  device        Pointer to device
         * @parma[in]  haddr         Host address of source to be copied into
         * @parma[in]  daddr         EVA address of destination to be copied from
         * @param[in]  bytes         Size of buffer to be copied
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_memcpy_to_host(hb_mc_device_t *device,
                                        void       *haddr,
                                        hb_mc_eva_t daddr,
                                        uint32_t bytes);


        /**
         * Sets memory to a give value starting from an address in device's DRAM.
         * @param[in]  device        Pointer to device
         * @parma[in]  eva           EVA address of destination 
         * @param[in]  val           Value to be written out
         * @param[in]  sz            The number of bytes to write into device DRAM
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        int hb_mc_device_memset (hb_mc_device_t *device,
                                 const hb_mc_eva_t *eva,
                                 uint8_t data,
                                 size_t sz); 





        /**
         * Frees memory on device DRAM
         * hb_mc_device_program_init() or hb_mc_device_program_init_binary() should
         * have been called before calling this function to set up a memory allocator.
         * @param[in]  device        Pointer to device
         * @param[out] eva           Eva address of the memory to be freed
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result)) 
        int hb_mc_device_free (hb_mc_device_t *device, hb_mc_eva_t eva);





        /**
         * Enqueues and schedules a kernel to be run on device
         * Takes the grid size, tile group dimensions, kernel name, argc,
         * argv* and the finish signal address, calls hb_mc_tile_group_enqueue
         * to initialize all tile groups for grid.
         * @param[in]  device        Pointer to device
         * @param[in]  grid_dim      X/Y dimensions of the grid to be initialized
         * @param[in]  tg_dim        X/Y dimensions of tile groups in grid
         * @param[in]  name          Kernel name to be executed on tile groups in grid
         * @param[in]  argc          Number of input arguments to kernel
         * @param[in]  argv          List of input arguments to kernel
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_kernel_enqueue (hb_mc_device_t *device,
                                       hb_mc_dimension_t grid_dim,
                                       hb_mc_dimension_t tg_dim,
                                       const char *name,
                                       const uint32_t argc,
                                       const uint32_t *argv);




        /**
         * Iterates over all tile groups inside device,
         * allocates those that fit in mesh and launches them. 
         * API remains in this function until all tile groups
         * have successfully finished execution.
         * @param[in]  device        Pointer to device
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_tile_groups_execute (hb_mc_device_t *device);



        /**
         * Deletes memory manager, device and manycore struct, and freezes all tiles in device.
         * @param[in]  device        Pointer to device
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_program_finish (hb_mc_device_t *device);


        /**
         * Deletes memory manager, device and manycore struct, and freezes all tiles in device on
         * all pods.
         * @param[in]  device        Pointer to device
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_finish (hb_mc_device_t *device);



        /***********/
        /* DMA API */
        /***********/
        typedef struct {
                hb_mc_eva_t d_addr; //!< Target EVA on the manycore
                const void* h_addr; //!< Target address on the host
                size_t      size;   //!< Size in bytes of the buffer to be copied
        } hb_mc_dma_htod_t;

        typedef struct {
                hb_mc_eva_t d_addr; //!< Target EVA on the manycore
                void*       h_addr; //!< Target address on the host
                size_t      size;   //!< Size in bytes of the buffer to be copied
        } hb_mc_dma_dtoh_t;

        /**
         * Copy data using DMA from the host to the device.
         * @param[in] device  Pointer to device
         * @param[in] jobs    Vector of host-to-device DMA jobs
         * @param[in] count   Number of host-to-device jobs
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_dma_to_device(hb_mc_device_t *device, const hb_mc_dma_htod_t *jobs, size_t count);

        /**
         * Copy data using DMA from the host to the device.
         * @param[in] device  Pointer to device
         * @param[in] jobs    Vector of device-to-host DMA jobs
         * @param[in] count   Number of device-to-host jobs
         */
        __attribute__((warn_unused_result))
        int hb_mc_device_dma_to_host(hb_mc_device_t *device, const hb_mc_dma_dtoh_t *jobs, size_t count);


        /*********************/
        /* Pod Interface DMA */
        /*********************/
        __attribute__((warn_unused_result))
        int hb_mc_device_pod_dma_to_device(hb_mc_device_t *device, hb_mc_pod_id_t pod, const hb_mc_dma_htod_t *jobs, size_t count);

        __attribute__((warn_unused_result))
        int hb_mc_device_pod_dma_to_host(hb_mc_device_t *device, hb_mc_pod_id_t pod, const hb_mc_dma_dtoh_t *jobs, size_t count);


        /**
         * Convenience macro for calling a CUDA function and handling an error return code.
         * @param[in] stmt  A C/C++ statement that evaluates to an integer return code.
         *
         * Example:
         * CUDA_CALL(hb_mc_device_malloc(&device, ...));
         *
         * The return code must be an integer defined in bsg_manycore_errno.h - otherwise behavior is undefined.
         * This macro will cause the invoking to return if an error code is returned.
         * An error message will be printing with the code statement that failed.
         */
#define BSG_CUDA_CALL(stmt)                                             \
        {                                                               \
                int __r = stmt;                                         \
                if (__r != HB_MC_SUCCESS) {                             \
                        bsg_pr_err("'%s' failed: %s\n", #stmt, hb_mc_strerror(__r)); \
                        return __r;                                     \
                }                                                       \
        }

#ifdef __cplusplus
}
#endif

#endif
