#pragma once
#include <bsg_manycore_errno.h>
#include <bsg_manycore.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore_cuda.h>
#include <bsg_manycore_tile.h>
#include <infra_hb/host/error.hpp>
#include <memory>
#include <vector>
#include <iostream>
#include <cstring>
#include <unordered_map>

#define VCACHE_BANKS 32
#define CACHE_LINE 16

/*
 * grid_init() -> after program_init_binary()
 * initialize a grid of tile groups.
 * I only want one, so make it 1x1 and make each tg 3x3.
 */
namespace hammerblade {
class Device {
public:
	/* Singleton object with automatic reference counting */
#if !defined(COSIM)
	typedef std::shared_ptr<Device>      Ptr;
	typedef std::shared_ptr<const Device> ConstPtr;
#else
	typedef Device* Ptr;
	typedef const Device* ConstPtr;
#endif
        class WriteJob {
        public:
                WriteJob(hb_mc_eva_t d_addr, const void *h_addr, size_t sz) :
                        _data(sz),
                        _d_addr(d_addr) {
                        memcpy(&_data[0], h_addr, sz);
                }

                hb_mc_dma_htod_t to_htod () const {
                        hb_mc_dma_htod_t job = { .d_addr = _d_addr, .h_addr = &_data[0], .size = _data.size()};
                        return job;
                }

        private:
                std::vector<unsigned char> _data;
                hb_mc_eva_t _d_addr;
        };

	/* no copy; no move */
	Device(Device && d)      = delete;
	Device(const Device & d) = delete;
	/* singleton object */
	static Device::Ptr GetInstance() {
		static Device::Ptr instance = nullptr;
		if (!instance)
#if !defined(COSIM)
			instance = Device::Ptr(new Device);
#else
			instance = new Device;
#endif
		return instance;
	}

	/* set the micro code data in vector */
	void setMicroCode(std::vector<unsigned char> && ucode) {
		_ucode = ucode;
		updateMicroCode();
	}


	/* void enqueue a CUDA task */
	/* should only be called after the micro code has been set */
	void enqueueJob(const std::string &kernel_name, hb_mc_dimension_t tile_dims,
			std::vector<uint32_t> argv) {
		int err;

		if (_ucode.empty())
			throw noUCodeError();
		/*
		 * this is necessary because of a bug in CUDA-lite
		 * where argv is not saved as a deep copy but as
		 * a pointer
		 */
		_argv_saves.push_back(std::move(argv));

		err = hb_mc_kernel_enqueue(_device,
				      hb_mc_dimension(1,1), /* grid of tile groups  */
				      tile_dims, /* tile group dimension */
				       // TODO: cast is required because of bug in CUDA-lite
				      (char*)kernel_name.c_str(),
				      _argv_saves.back().size(),
				      _argv_saves.back().data());

		if (err != HB_MC_SUCCESS)
			throw hammerblade::manycore_runtime_error(err);


	}

	/* void enqueue a CUDA task on a 1x1 tile group */
	/* should only be called after the micro code has been set */
	void enqueueSerialJob(const std::string &kernel_name,
			std::vector<uint32_t> argv) {
		int err;

		if (_ucode.empty())
			throw noUCodeError();
		/*
		 * this is necessary because of a bug in CUDA-lite
		 * where argv is not saved as a deep copy but as
		 * a pointer
		 */
		_argv_saves.push_back(std::move(argv));

		err = hb_mc_kernel_enqueue(_device,
				      hb_mc_dimension(1,1), /* grid of tile groups  */
				      hb_mc_dimension(1,1), /* tile group dimension */
				       // TODO: cast is required because of bug in CUDA-lite
				      (char*)kernel_name.c_str(),
				      _argv_saves.back().size(),
				      _argv_saves.back().data());

		if (err != HB_MC_SUCCESS)
			throw hammerblade::manycore_runtime_error(err);


	}

	/*
	 * run enqueued jobs
	 */
	void runJobs() {
		int err;

		if (_ucode.empty())
			throw noUCodeError();

		err = hb_mc_device_tile_groups_execute(_device);
		if (err != HB_MC_SUCCESS)
			throw hammerblade::manycore_runtime_error(err);
		/*
		 * argv saves can now be released
		 */
		_argv_saves.clear();
	}

	/*
	 * allocate some application memory on the device
	 */
	hb_mc_eva_t malloc(hb_mc_eva_t sz) {
		if (_ucode.empty())
			throw noUCodeError();

		hb_mc_eva_t mem;
		int err = hb_mc_device_malloc(_device, sz, &mem);
		if (err != HB_MC_SUCCESS)
			throw hammerblade::manycore_runtime_error(err);

		return mem;
	}

        hb_mc_eva_t malloc_align(hb_mc_eva_t sz) {
          hb_mc_eva_t align = VCACHE_BANKS * CACHE_LINE * sizeof(int);                     
          hb_mc_eva_t ptr = this->malloc(sz + align);
          hb_mc_eva_t rem = ptr % align;
          ptr += align;
          ptr -= rem;
          return ptr;
        }

	/*
	 * free application memory on the device
	 */
	void free(hb_mc_eva_t mem) {
		if (_ucode.empty())
			throw noUCodeError();

		int err = hb_mc_device_free(_device, mem);
		if (err != HB_MC_SUCCESS)
			throw hammerblade::manycore_runtime_error(err);
	}

	/*
	 * write data to application memory
	 */
	void write(hb_mc_eva_t dst, const void *src, hb_mc_eva_t sz) {
		if (_ucode.empty())
			throw noUCodeError();
		int err = hb_mc_device_memcpy(_device,
						      (void*)dst,
						      src,
						      sz,
						      HB_MC_MEMCPY_TO_DEVICE);
		if (err != HB_MC_SUCCESS)
			throw hammerblade::manycore_runtime_error(err);
	}

	void enqueue_write_task(hb_mc_eva_t dst, const void *src, hb_mc_eva_t sz) {
		if (_ucode.empty())
			throw noUCodeError();
		if (!hb_mc_manycore_supports_dma_write(_device->mc)) {
			std::cerr << "dma not supported" << std::endl;
			write(dst, src, sz);
			return;
		}

                _write_jobs.push_back(WriteJob(dst, src, sz));
	}

	void write_dma() {
                std::vector<hb_mc_dma_htod_t> _jobs;
                for (auto & j : _write_jobs) _jobs.push_back(j.to_htod());
		int err = hb_mc_device_dma_to_device(_device, &_jobs[0], _jobs.size());

		_write_jobs.clear();
		if (err != HB_MC_SUCCESS)
			throw hammerblade::manycore_runtime_error(err);
	}

	/*
	 * read data from application memory
	 */
	void read(void *dst, hb_mc_eva_t src, hb_mc_eva_t sz) {
		if (_ucode.empty())
			throw noUCodeError();
		int err = hb_mc_device_memcpy(_device,
					      dst,
					      (const void*)src,
					      sz,
					      HB_MC_MEMCPY_TO_HOST);
		if (err != HB_MC_SUCCESS)
			throw hammerblade::manycore_runtime_error(err);

	}

	void enqueue_read_task(void *dst, hb_mc_eva_t src, hb_mc_eva_t sz) {
		if (_ucode.empty())
			throw noUCodeError();
		if (!hb_mc_manycore_supports_dma_read(_device->mc)) {
			std::cerr << "dma not supported" << std::endl;
			read(dst, src, sz);
			return;
		}
		hb_mc_dma_dtoh_t dtoh_job = {
			.d_addr = src,
			.h_addr = dst,
			.size   = sz
		};
		_read_jobs.push_back(dtoh_job);
	}

	void read_dma() {
		int err = hb_mc_device_dma_to_host(_device, &_read_jobs[0], _read_jobs.size());
		_read_jobs.clear();
		if (err != HB_MC_SUCCESS)
			throw hammerblade::manycore_runtime_error(err);
	}

private:
	hammerblade::runtime_error noUCodeError() const {
		return hammerblade::runtime_error("Device not initialized with microcode");
	}
	static char * CUDA_DEVICE_NAME() {
		static char cuda_device_name [] = "GraphIt";
		return cuda_device_name;
	}
	static char * CUDA_PROGRAM_NAME() {
		static char cuda_program_name [] = "GraphIt Micro Code";
		return cuda_program_name;
	}
	static char *CUDA_ALLOC_NAME() {
		static char cuda_alloc_name [] = "GraphIt Allocator";
		return cuda_alloc_name;
	}

        /* update the micro code on the device */
        void updateMicroCode() {
                int err;

                // necessary because device_program_exit() will
                // free ucode_ptr (BUG)
                unsigned char *ucode_ptr = new unsigned char[_ucode.size()];
                memcpy(ucode_ptr, _ucode.data(), _ucode.size()*sizeof(unsigned char));

                err = hb_mc_device_program_init_binary(_device,
                                                       CUDA_PROGRAM_NAME(),
                                                       ucode_ptr,
                                                       _ucode.size(),
                                                       CUDA_ALLOC_NAME(),
                                                       0);
                if (err != HB_MC_SUCCESS)
                        throw hammerblade::manycore_runtime_error(err);

		// all symbol addresses are now invalid
		_symbol_addresses.clear();
        }

private:
        template <bool FREEZE>
        void freeze_unfreeze_cores() {
                hb_mc_manycore_t *mc = _device->mc;
                const hb_mc_config_t*cfg = hb_mc_manycore_get_config(mc);
                hb_mc_dimension_t base = hb_mc_config_get_origin_vcore(cfg);
                for (hb_mc_idx_t x = 0; x < hb_mc_config_get_dimension_vcore(cfg).x; x++) {
                        for (hb_mc_idx_t y = 0; y < hb_mc_config_get_dimension_vcore(cfg).y; y++) {
                                hb_mc_coordinate_t xy = hb_mc_coordinate(base.x+x, base.y+y);
                                int r = FREEZE
                                        ? hb_mc_tile_freeze(mc, &xy)
                                        : hb_mc_tile_unfreeze(mc, &xy);
                                if (r != HB_MC_SUCCESS)
                                        throw hammerblade::manycore_runtime_error(r);
                        }
                }
        }

public:
        void freeze_cores() { freeze_unfreeze_cores<true>(); }
        void unfreeze_cores() { freeze_unfreeze_cores<false>(); }
        hb_mc_device_t * getDevice() { return _device; }

protected:
	/* singleton; constructor is protected */
	Device() : _device(nullptr) {
		int err;

		/* allocate and initialize CUDA-lite handles */
		_device = new hb_mc_device_t;

		//err = hb_mc_device_init_custom_dimensions(_device, CUDA_DEVICE_NAME(), 0, hb_mc_dimension(4,4));
		err = hb_mc_device_init(_device, CUDA_DEVICE_NAME(), 0);
		if (err != HB_MC_SUCCESS)
			throw hammerblade::manycore_runtime_error(err);

		/* make sure ucode is cleared */
		_ucode.clear(); // probably not necessary
	}
public:
	~Device() {
		int err;

		/* cleanup and free CUDA-lite handles */
		err = hb_mc_device_finish(_device);
		if (err != HB_MC_SUCCESS)
			throw hammerblade::manycore_runtime_error(err);

		delete _device;
	}

public:
        /*
           symbol lookup API
           note that the address is invalid after setMicroCode() is called
         */
        hb_mc_eva_t findSymbolAddress(const std::string symbol_name) {
                // first check if we have looked for this symbol before
                auto addr_it = _symbol_addresses.find(symbol_name);
                if (addr_it != _symbol_addresses.end())
                        return addr_it->second;

                // otherwise consult the manycore API
                if (_ucode.empty())
                        throw noUCodeError();

                hb_mc_eva_t sym_addr;
                int err = hb_mc_loader_symbol_to_eva(_ucode.data(),
                                                     _ucode.size(),
                                                     symbol_name.c_str(),
                                                     &sym_addr);
                if (err != HB_MC_SUCCESS)
                        throw hammerblade::manycore_runtime_error(err);

                _symbol_addresses[symbol_name] = sym_addr;

                return sym_addr;
        }
private:
        std::unordered_map<std::string, hb_mc_eva_t> _symbol_addresses;
	std::vector< std::vector <uint32_t> > _argv_saves;
	std::vector<unsigned char> _ucode;
	hb_mc_device_t * _device;
	std::vector<WriteJob> _write_jobs;
	std::vector<hb_mc_dma_dtoh_t> _read_jobs;
};
}
