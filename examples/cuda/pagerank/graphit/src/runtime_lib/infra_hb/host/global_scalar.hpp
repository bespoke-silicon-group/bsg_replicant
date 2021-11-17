#pragma once
#include <infra_hb/host/device.hpp>
#include <bsg_manycore_cuda.h>
#include <string>
#include <type_traits>

namespace hammerblade {
/**
 * A wrapper class for accessing global variables statically allocated on the device.
 */
template <typename T>
class GlobalScalar {
        static_assert(std::is_scalar<T>::value && sizeof(T) == sizeof(hb_mc_eva_t),
                      "type template for GlobalScalar must be built-in "
                      "scalar type and the size must match HammerBlade machine word size");
public:
        /**
         * Empty constructor.
        */
#if !defined(COSIM)
        GlobalScalar() : GlobalScalar("") {}
#else
        GlobalScalar() {}
#endif
        /*
         * Constructor from std::string.
         * param[in] varname is the name of the global on the device.
         */
        explicit GlobalScalar(const std::string & varname) :
                _device(Device::GetInstance()),
                _varname(varname) {}

        /**
         * Set the value of a global.
         * @param[in] the new value of the global.
         */
        void set(const T &v) {
                Device::Ptr device = Device::GetInstance();
                hb_mc_eva_t addr = device->findSymbolAddress(_varname);
                device->write(addr, &v, sizeof(T));

        }
        /**
         * Get the value of a global.
         * @return the value of the global.
         */
        T get () const {
                Device::Ptr device = Device::GetInstance();
                hb_mc_eva_t addr = device->findSymbolAddress(_varname);
                T v;
                device->read(&v, addr, sizeof(T));
                return v;
        }
        /**
         * Get the size of a global.
         * @return the size in bytes of the global.
         */
        size_t scalar_size() const { return sizeof(T); }
private:
        std::string _varname;
        Device::Ptr _device;
};

/**
 * A function for doing buffered reads from a global pointer on the manycore.
 * @param dst  A buffer on the host to load read data.
 * @param src  A global pointer on the manycore - points to a manycore buffer from which to read.
 * @param cnt  The number of elements to read.
 */

template <typename T>
void read_global_buffer(T *dst, const GlobalScalar<hb_mc_eva_t>& glbl_ptr, hb_mc_eva_t cnt)
{
        // read the value of the pointer on the device
        auto src = glbl_ptr.get();
        auto device = Device::GetInstance();

        // read from the device starting at the eva read
        device->read(dst, src, cnt * sizeof(T));
}

template <typename T>
void read_global_buffer_dma(T *dst, const GlobalScalar<hb_mc_eva_t>& glbl_ptr, hb_mc_eva_t cnt)
{
        // read the value of the pointer on the device
        auto src = glbl_ptr.get();
        auto device = Device::GetInstance();

        // read from the device starting at the eva read
        device->enqueue_read_task(dst, src, cnt * sizeof(T));
}

template <typename T>
void write_global_buffer(T *host, const GlobalScalar<hb_mc_eva_t>& glbl_ptr, hb_mc_eva_t cnt)
{
        // read the value of the pointer on the device
        auto dst = glbl_ptr.get();
        auto device = Device::GetInstance();

        // read from the device starting at the eva read
        device->write(dst, (const void*)host, cnt * sizeof(T));
}

template <typename T>
void write_global_buffer_dma(T *host, const GlobalScalar<hb_mc_eva_t>& glbl_ptr, hb_mc_eva_t cnt)
{
        // read the value of the pointer on the device
        auto dst = glbl_ptr.get();
        auto device = Device::GetInstance();

        // read from the device starting at the eva read
        device->enqueue_write_task(dst, (const void*)host, cnt * sizeof(T));
}

template <typename T>
void init_global_array(size_t len, GlobalScalar<hb_mc_eva_t>& glbl_ptr)
{
  hb_mc_eva_t mem;
  auto device = Device::GetInstance();

  mem = device->malloc(len * sizeof(T));
  glbl_ptr.set(mem);
}

//method to insert a value to a global scalar
template <typename T>
void insert_val(size_t pos, const T & val, const GlobalScalar<hb_mc_eva_t>& glbl_ptr)
{
  auto dst = glbl_ptr.get();
  auto device = Device::GetInstance();

  device->write(dst + (pos * sizeof(T)), (const void *)&val, sizeof(T));
}

template <typename T>
void insert_val_dma(size_t pos, const T & val, const GlobalScalar<hb_mc_eva_t>& glbl_ptr)
{
  auto dst = glbl_ptr.get();
  auto device = Device::GetInstance();

  device->enqueue_write_task(dst + (pos * sizeof(T)), (const void *)&val, sizeof(T));
}

template <typename T>
void assign_val(size_t start, size_t end, const T & val, const GlobalScalar<hb_mc_eva_t>& glbl_ptr)
{
  auto dst = glbl_ptr.get();
  auto device = Device::GetInstance();
  for (size_t i = start; i < end; i++)
    device->write(dst + (i * sizeof(T)), (const void *)&val, sizeof(T));
}

template <typename T>
void assign_val_dma(size_t start, size_t end, const T & val, const GlobalScalar<hb_mc_eva_t>& glbl_ptr)
{
  auto dst = glbl_ptr.get();
  auto device = Device::GetInstance();
  for (size_t i = start; i < end; i++)
    device->enqueue_write_task(dst + (i * sizeof(T)), (const void *)&val, sizeof(T));
}

template <typename T>
void swap_global_arrays(GlobalScalar<hb_mc_eva_t> src, GlobalScalar<hb_mc_eva_t>& dst)
{
  auto dst_val = dst.get();
  auto src_val = src.get();
  dst.set(src_val);
  src.set(src_val);
}

}
