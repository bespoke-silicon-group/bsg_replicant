#pragma once
#include <bsg_manycore_errno.h>
#include <stdexcept>
#include <string>

namespace hammerblade {

class runtime_error : public std::runtime_error {
public:
	explicit runtime_error(const std::string &what) :
		std::runtime_error(what) {}
};

class manycore_runtime_error : public hammerblade::runtime_error {
public:
	explicit manycore_runtime_error(int error) :
		hammerblade::runtime_error(hb_mc_strerror(error)) {}
};

}
