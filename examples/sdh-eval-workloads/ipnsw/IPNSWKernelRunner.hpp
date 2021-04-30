#pragma once
#include "HammerBlade.hpp"
#include <memory>
#include <string>
namespace ipnsw {
    class IPNSWRunner; // forward declaration

    class IPNSWKernelRunner {
    public:
        using HammerBlade = hammerblade::host::HammerBlade;
        using Dim = hammerblade::host::Dim;
        IPNSWKernelRunner() {}

    protected:
        virtual std::string kernelName(const IPNSWRunner & runner) const =0;
        virtual std::vector<hb_mc_eva_t> argv(const IPNSWRunner & runner) const =0;
        virtual Dim gd(const IPNSWRunner &runner) const = 0;
        virtual Dim tgd(const IPNSWRunner &runner) const = 0;

    public:
        void runKernel(IPNSWRunner &runner) {
            HammerBlade::Ptr hb = HammerBlade::Get();
            hb->push_jobv(gd(runner),
                          tgd(runner),
                          kernelName(runner),
                          argv(runner));
            hb->exec();
        }
    };

}
