#pragma once
#include "IPNSWFactory.hpp"
#include "IProductUBmkKernelRunner.hpp"
#include "IProductUBmkResultReader.hpp"
namespace ipnsw {
    class IProductUBmkFactory : public IPNSWFactory {
    public:
        IProductUBmkFactory(int iterations = 10):
            _iterations(iterations) {
        }

    private:
        IPNSWKernelRunner *_KernelRunner() const { return new IProductUBmkKernelRunner(_iterations); }
        IPNSWResultReader *_ResultReader() const { return new IProductUBmkResultReader; }

        int _iterations;
    };
}

