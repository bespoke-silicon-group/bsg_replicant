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

#define LOCAL_PCI_HANDLE_INIT (-1)
#define LOCAL_PCI_HANDLE_DEFAULT (0)

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/file.h>
const size_t MAP_SIZE=32768UL;
#define DEVICE_NAME_FORMAT "/dev/xdmaBSG%d_user"
#define DEVICE_H2C_NAME_FORMAT "/dev/xdmaBSG%d_h2c_0"
#define DEVICE_C2H_NAME_FORMAT "/dev/xdmaBSG%d_c2h_0"

#include <bsg_manycore_mmio.h>
#include <cstring>

int fd;
int fd_h2c;
int fd_c2h;

/**
 * Initialize MMIO for operation
 * @param[in]  mmio   MMIO pointer to initialize
 * @param[in]  handle PCI BAR handle to map
 * @param[in]  id     ID which selects the physical hardware from which this manycore is configured
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_mmio_init(hb_mc_mmio_t *mmio,
                           int* handle,
                           hb_mc_manycore_id_t id)
{
        int r = HB_MC_FAIL, err;

        // negative IDs are invalid at the moment
        if (id < 0) {
                mmio_pr_err((*mmio), "Failed to init MMIO: invalid ID\n");
                return HB_MC_INVALID;
        }
        *handle = LOCAL_PCI_HANDLE_DEFAULT;

        char device_name_buffer[64];
        sprintf(device_name_buffer, DEVICE_NAME_FORMAT, id);
        const char* device_name = (const char*) device_name_buffer;

        char device_h2c_name_buffer[64];
        sprintf(device_h2c_name_buffer, DEVICE_H2C_NAME_FORMAT, id);
        const char* device_h2c_name = (const char*) device_h2c_name_buffer;

        char device_c2h_name_buffer[64];
        sprintf(device_c2h_name_buffer, DEVICE_C2H_NAME_FORMAT, id);
        const char* device_c2h_name = (const char*) device_c2h_name_buffer;

        if ((fd = open(device_name, O_RDWR | O_SYNC)) == -1) {
                bsg_pr_err("Failed to open device: %s\n", device_name);
                goto cleanup;
            }
        else if ((fd_h2c = open(device_h2c_name, O_RDWR | O_NONBLOCK)) < 0) {
                bsg_pr_err("Failed to open device: %s\n", device_h2c_name);
                goto cleanup;
            }
        else if ((fd_c2h = open(device_c2h_name, O_RDWR | O_NONBLOCK)) < 0) {
                bsg_pr_err("Failed to open device: %s\n", device_c2h_name);
                goto cleanup;
            }
        else {
                if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
                    bsg_pr_err("Failed to lock device: %s\n", device_name);
                    goto cleanup;
                }
                if (flock(fd_h2c, LOCK_EX | LOCK_NB) < 0) {
                    bsg_pr_err("Failed to lock device: %s\n", device_h2c_name);
                    goto cleanup;
                }
                if (flock(fd_c2h, LOCK_EX | LOCK_NB) < 0) {
                    bsg_pr_err("Failed to lock device: %s\n", device_c2h_name);
                    goto cleanup;
                }
                mmio->p = (uintptr_t) mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                if(mmio->p == (uintptr_t)-1) {
                    bsg_pr_err("Failed to mmap device: %s\n", device_name);
                    goto cleanup;
                }
                bsg_pr_info("Device %s:%d is opened and memory mapped at 0x%x\n", device_name, fd, mmio->p);
                bsg_pr_info("Device %s:%d is opened\n", device_h2c_name, fd_h2c);
                bsg_pr_info("Device %s:%d is opened\n", device_c2h_name, fd_c2h);
        }
        r = HB_MC_SUCCESS;
        mmio_pr_dbg(mmio, "%s: mmio = 0x%" PRIxPTR "\n", __func__, mmio->p);
        goto done;

 cleanup:
        if (munmap((void**)&mmio->p, MAP_SIZE) == -1) {
            mmio_pr_err((*mmio), "Failed to munmap MMIO!\n", __func__);
        }
        if (flock(fd, LOCK_UN) == -1) {
            bsg_pr_err("Failed to unlock device: %s\n", device_name);
        }
        if (flock(fd_h2c, LOCK_UN) == -1) {
            bsg_pr_err("Failed to unlock device: %s\n", device_h2c_name);
        }
        if (flock(fd_c2h, LOCK_UN) == -1) {
            bsg_pr_err("Failed to unlock device: %s\n", device_c2h_name);
        }
        close(fd);
        close(fd_h2c);
        close(fd_c2h);
        *handle = LOCAL_PCI_HANDLE_INIT;
 done:
        return r;
}

/**
 * Clean up MMIO for termination
 * @param[in]  mmio   MMIO pointer to clean up
 * @param[in]  handle PCI BAR handle to unmap
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_mmio_cleanup(hb_mc_mmio_t *mmio,
                              int *handle)
{
        int err;

        if (flock(fd, LOCK_UN) == -1) {
            bsg_pr_err("Failed to unlock device\n");
        }
        if (flock(fd_h2c, LOCK_UN) == -1) {
            bsg_pr_err("Failed to unlock device\n");
        }
        if (flock(fd_c2h, LOCK_UN) == -1) {
            bsg_pr_err("Failed to unlock device\n");
        }
        close(fd);
        close(fd_h2c);
        close(fd_c2h);
        if (*handle == LOCAL_PCI_HANDLE_INIT)
                return HB_MC_SUCCESS;

        *handle = LOCAL_PCI_HANDLE_INIT;
        (*mmio).p = reinterpret_cast<uintptr_t>(nullptr);
        return HB_MC_SUCCESS;
}

/**
 * Write data to manycore hardware at a given AXI Address
 * @param[in]  mmio     An MMIO pointer instance initialized with hb_mc_mmio_init()
 * @param[in]  offset An offset into the manycore's MMIO address space
 * @param[in]  vp     A pointer to a value to be written out
 * @param[in]  sz     Number of bytes in the pointer to be written out
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_mmio_read(hb_mc_mmio_t mmio, uintptr_t offset,
                           void *vp, size_t sz)
{
        unsigned char *addr = reinterpret_cast<unsigned char *>(mmio.p);
        uint32_t tmp;

        if (addr == nullptr) {
                mmio_pr_err((mmio), "%s: Failed: MMIO not initialized", __func__);
                return HB_MC_UNINITIALIZED;
        }

        // check that the address is aligned to a four byte boundar
        if (offset % 4) {
                mmio_pr_err((mmio), "%s: Failed: 0x%" PRIxPTR " "
                            "is not aligned to 4 byte boundary\n",
                            __func__, offset);
                return HB_MC_UNALIGNED;
        }

        addr = &addr[offset];

        tmp = *(volatile uint32_t *)addr;

        switch (sz) {
        case 4:
                *(uint32_t*)vp = tmp;
                break;
        case 2:
                *(uint16_t*)vp = tmp;
                break;
        case 1:
                *(uint8_t*)vp  = tmp;
                break;
        default:
                mmio_pr_err((mmio), "%s: Failed: invalid load size (%zu)\n", __func__, sz);
                return HB_MC_INVALID;
        }
        //bsg_pr_info("0_%08x_%08x\n", offset, *(uint32_t *)vp);

        return HB_MC_SUCCESS;
}

/**
 * Write data to manycore hardware at a given AXI Address
 * @param[in]  mmio     An MMIO pointer instance initialized with hb_mc_mmio_init()
 * @param[in]  offset An offset into the manycore's MMIO address space
 * @param[in]  vp     A pointer to a value to be written out
 * @param[in]  sz     Number of bytes in the pointer to be written out
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_mmio_write(hb_mc_mmio_t mmio, uintptr_t offset,
                            void *vp, size_t sz)
{
        unsigned char *addr = reinterpret_cast<unsigned char *>(mmio.p);
        uint32_t tmp;
        //bsg_pr_info("1_%08x_%08x\n", offset, *(uint32_t *)vp);

        if (addr == nullptr) {
                mmio_pr_err((mmio), "%s: Failed: MMIO not initialized", __func__);
                return HB_MC_UNINITIALIZED;
        }

        // check that the address is aligned to a four byte boundary
        if (offset % 4) {
                mmio_pr_err((mmio), "%s: Failed: 0x%" PRIxPTR " "
                            "is not aligned to 4 byte boundary\n",
                            __func__, offset);
                return HB_MC_UNALIGNED;
        }

        addr = &addr[offset];

        switch (sz) {
        case 4:
                tmp = *(uint32_t *)vp;
                break;
        case 2:
                tmp = *(uint16_t*)vp;
                break;
        case 1:
                tmp = *(uint8_t*)vp;
                break;
        default:
                mmio_pr_err((mmio), "%s: Failed: invalid load size (%zu)\n", __func__, sz);
                return HB_MC_INVALID;
        }

        *(volatile uint32_t *)addr = tmp;

        return HB_MC_SUCCESS;
}

/**
 * Signal the hardware to start a bulk transfer over the network
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_start_bulk_transfer(hb_mc_manycore_t *mc){
        return HB_MC_SUCCESS;
}

/**
 * Signal the hardware to end a bulk transfer over the network
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_finish_bulk_transfer(hb_mc_manycore_t *mc){
        return HB_MC_SUCCESS;
}

int hb_mc_mmio_write_h2c(uint64_t addr, uint32_t align, const char *data, size_t sz)
{
    ssize_t rc;
    off_t offset = 0;
    size_t count = 0;
    size_t size = 0;
    size_t size_padded = 0;

    // check that the address is aligned to <align> byte boundary
    if (addr % align) {
        bsg_pr_err("Address %llu is not aligned to %d byte boundary.\n", addr, align);
        return HB_MC_UNALIGNED;
    }

    char *allocated = NULL;
    char *buffer = NULL;
    size_t step_size = 65536;
    posix_memalign((void **)&allocated, 4096 /*alignment */ , step_size + 4096);
    if (!allocated) {
        bsg_pr_err("Failed to allocate memory for h2c, OOM %lu.\n", step_size + 4096);
        return HB_MC_UNINITIALIZED;
    }
    buffer = allocated + offset;

    offset = addr;
    while (count < sz) {
        if (count + step_size > sz) {
            size = sz - count;
            size_padded = size;
            // add padding to size, if size is not multiple of <align>
            if (size % align) {
                size_padded = size + (align - (size % align));
            }
        } else {
            size = step_size;
            size_padded = size;
        }
        memcpy(buffer, data+count, size);
        for (size_t i = size; i < size_padded; i++) {
            buffer[i] = 0;
        }
        if (offset) {
            rc = lseek(fd_h2c, offset, SEEK_SET);
            if (rc != offset) {
                bsg_pr_err("h2c, seek off 0x%lx != 0x%lx.\n", rc, offset);
                perror("seek file");
                return HB_MC_FAIL;
            }
        }
        rc = write(fd_h2c, buffer, size_padded);
        if (rc != size_padded) {
            bsg_pr_err("h2c, W off 0x%lx, 0x%lx != 0x%lx.\n", offset, rc, size_padded);
            perror("write file");
            return HB_MC_FAIL;
        }
        offset += size;
        count += size;
    }

    free(allocated);
    return HB_MC_SUCCESS;
}

int hb_mc_mmio_read_c2h(uint64_t addr, uint32_t align, char *data, size_t sz)
{
    ssize_t rc;
    off_t offset = 0;
    size_t count = 0;
    size_t size = 0;
    size_t size_padded = 0;

    // check that the address is aligned to <align> byte boundary
    if (addr % align) {
        bsg_pr_err("Address %llu is not aligned to %d byte boundary.\n", addr, align);
        return HB_MC_UNALIGNED;
    }

    char *allocated = NULL;
    char *buffer = NULL;
    size_t step_size = 65536;
    posix_memalign((void **)&allocated, 4096 /*alignment */ , step_size + 4096);
    if (!allocated) {
        bsg_pr_err("Failed to allocate memory for c2h, OOM %lu.\n", step_size + 4096);
        return HB_MC_UNINITIALIZED;
    }
    buffer = allocated + offset;

    offset = addr;
    while (count < sz) {
        if (count + step_size > sz) {
            size = sz - count;
            size_padded = size;
            // add padding to size, if size is not multiple of <align>
            if (size % align) {
                size_padded = size + (align - (size % align));
            }
        } else {
            size = step_size;
            size_padded = size;
        }
        if (offset) {
            rc = lseek(fd_c2h, offset, SEEK_SET);
            if (rc != offset) {
                bsg_pr_err("c2h, seek off 0x%lx != 0x%lx.\n", rc, offset);
                perror("seek file");
                return HB_MC_FAIL;
            }
        }
        rc = read(fd_c2h, buffer, size_padded);
        if (rc != size_padded) {
            bsg_pr_err("c2h, W off 0x%lx, 0x%lx != 0x%lx.\n", offset, rc, size_padded);
            perror("read file");
            return HB_MC_FAIL;
        }
        memcpy(data+count, buffer, size);
        offset += size;
        count += size;
    }

    free(allocated);
    return HB_MC_SUCCESS;
}
