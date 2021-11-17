#ifndef GRAPHIT_HB_SCHEDULE
#define GRAPHIT_HB_SCHEDULE

#include <assert.h>

namespace graphit {
namespace fir {
namespace hb_schedule {

enum hb_schedule_options {
    PUSH,
    PULL,
    BLOCKING,
    ALIGNED,
    EDGE_BASED,
    VERTEX_BASED
};

class HBSchedule {
public:
    virtual ~HBSchedule() = default;
};

class SimpleHBSchedule: public HBSchedule {
public:
    enum class DirectionType {
        PUSH,
        PULL
    };

    enum class HBLoadBalanceType {
        VERTEX_BASED,
        EDGE_BASED,
        ALIGNED,
        BLOCKED
    };

private:
public:
    HBLoadBalanceType hb_load_balance_type;
    DirectionType direction_type;

public:
    SimpleHBSchedule() {
        hb_load_balance_type = HBLoadBalanceType::VERTEX_BASED;
        direction_type = DirectionType::PUSH;
    }

    void configDirection(enum hb_schedule_options o) {
        switch(o) {
            case PUSH:
                direction_type = DirectionType::PUSH;
                break;
            case PULL:
                direction_type = DirectionType::PULL;
                break;
            default:
                assert(false && "Invalid option for configDirection");
                break;
        }
    }

    void configLoadBalance(enum hb_schedule_options o) {
        switch(o) {
            case VERTEX_BASED:
                hb_load_balance_type = HBLoadBalanceType::VERTEX_BASED;
                break;
            case EDGE_BASED:
                hb_load_balance_type = HBLoadBalanceType::EDGE_BASED;
                break;
            case ALIGNED:
                hb_load_balance_type = HBLoadBalanceType::ALIGNED;
                break;
            case BLOCKING:
                hb_load_balance_type = HBLoadBalanceType::BLOCKED;
                break;
            default:
                assert(false && "Invalid option for configLoadBalance");
                break;
        }
    }

};

}
}
}

#endif
