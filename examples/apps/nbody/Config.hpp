#pragma once

// DR: Configuration arguments
struct Config {
        const float dtime; // length of one time step
        const float eps;   // potential softening parameter
        const float tol;   // tolerance for stopping recursion, <0.57 to bound error
        const float dthf, epssq, itolsq;
        Config()
                : dtime(0.5), eps(0.05), tol(0.05), // 0.025),
                  dthf(dtime * 0.5), epssq(eps * eps), itolsq(1.0 / (tol * tol)) {}
};
