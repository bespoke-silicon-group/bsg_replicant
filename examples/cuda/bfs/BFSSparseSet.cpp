#include "bfs/BFSSparseSet.hpp"
namespace BFS {
    void BFSSparseSet::formatOnDevice(kernel_sparse_set_ptr_t dev_ptr) {
        //_set_dev = _hb->alloc(sizeof(*_kset) + _kset->set_cap*sizeof(int));
        _hb->push_write(dev_ptr, _kset, sizeof(*_kset) + _kset->set_cap*sizeof(int));        
    }
    void BFSSparseSet::formatOnDevice() {
        _set_dev = _hb->alloc(sizeof(*_kset) + _kset->set_cap*sizeof(int));
        _hb->push_write(_set_dev, _kset, sizeof(*_kset) + _kset->set_cap*sizeof(int));        
    }
    void BFSSparseSet::updateFromDevice() {
        _hb->push_read(_set_dev, _kset, sizeof(*_kset) + _kset->set_cap*sizeof(int));
    }
}
