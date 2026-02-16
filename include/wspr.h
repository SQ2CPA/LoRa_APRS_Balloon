#include <Arduino.h>

#define SI_CLK0_CONTROL 16 // Register definitions
#define SI_CLK1_CONTROL 17
#define SI_CLK2_CONTROL 18
#define SI_SYNTH_PLL_A 26
#define SI_SYNTH_PLL_B 34
#define SI_SYNTH_MS_0 42
#define SI_SYNTH_MS_1 50
#define SI_SYNTH_MS_2 58
#define SI_PLL_RESET 177

#define SI_R_DIV_1 0b00000000 // R-division ratio definitions
#define SI_R_DIV_2 0b00010000
#define SI_R_DIV_4 0b00100000
#define SI_R_DIV_8 0b00110000
#define SI_R_DIV_16 0b01000000
#define SI_R_DIV_32 0b01010000
#define SI_R_DIV_64 0b01100000
#define SI_R_DIV_128 0b01110000

#define SI_CLK_SRC_PLL_A 0b00000000
#define SI_CLK_SRC_PLL_B 0b00100000

#define WSPR_SYMBOL_COUNT 162

#define WSPR_FREQ4m 7009250000ULL  // 4m     70.092,500MHz
#define WSPR_FREQ6m 5029450000ULL  // 6m     50.294,500MHz
#define WSPR_FREQ10m 2812610000ULL // 10m    28.126,100MHz
#define WSPR_FREQ12m 2492610000ULL // 12m    24.926,100MHz
#define WSPR_FREQ15m 2109610000ULL // 15m    21.096.100MHz
#define WSPR_FREQ17m 1810610000ULL // 17m    18.106,100MHz
#define WSPR_FREQ20m 1409710000ULL // 20m    14.097,100MHz
#define WSPR_FREQ30m 1014020000ULL // 30m    10.140,200MHz
#define WSPR_FREQ40m 704010000ULL  // 40m     7.040,100MHz
#define WSPR_FREQ80m 357010000ULL  // 80m     3.570,100MHz
#define WSPR_FREQ160m 183810000ULL // 160m    1.838,100MHz
#define WSPR_FREQ630m 47570000ULL  // 630m      475.700kHz
#define WSPR_FREQ2190m 13750000ULL // 2190m     137.500kHz

#define FactorySpace true
#define UserSpace false

#define UMesCurrentMode 1
#define UMesLocator 2
#define UMesTime 3
#define UMesGPSLock 4
#define UMesNoGPSLock 5
#define UMesFreq 6
#define UMesTXOn 7
#define UMesTXOff 8
#define UMesLPF 9
#define UMesVCC 10
#define UMesWSPRBandCycleComplete 11

#define rot(x, k) ((x << k) | (x >> (32 - k)))

namespace WSPR
{
    void setup();
    void prepareWSPR(uint32_t altitude);
    void sendWSPR(int minute);
    bool isInTimeslot(int minute, int second);
    void disableTX();
    void debug();
    bool isAvailable();
    void startupTone();
}