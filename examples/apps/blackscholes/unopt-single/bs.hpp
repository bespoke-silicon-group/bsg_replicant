#pragma once
typedef struct OptionData_ {
        float s;          // spot price
        float strike;     // strike price
        float r;          // risk-free interest rate
        float divq;       // dividend rate
        float v;          // volatility
        float t;          // time to maturity or option expiration in years 
                           //     (1yr = 1.0, 6mos = 0.5, 3mos = 0.25, ..., etc)  
        char OptionType;   // Option type.  "P"=PUT, "C"=CALL
        float divs;       // dividend vals (not used in this test)
        float DGrefval;   // DerivaGem Reference Value
} OptionData;

float BlkSchlsEqEuroNoDiv( float sptprice,
                           float strike, float rate, float volatility,
                           float time, int otype, float timet );
