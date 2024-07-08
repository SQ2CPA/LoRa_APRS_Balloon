#include <Arduino.h>
#include "wspr.h"
#include <Wire.h>
#include "configuration.h"
#include "utils.h"

const uint8_t LP_A = 0;
const uint8_t LP_B = 1;
const uint8_t LP_C = 2;
const uint8_t LP_D = 3;

uint8_t address = 96;

uint8_t power1, power2;

uint8_t txBuffer[WSPR_SYMBOL_COUNT];

extern Configuration Config;

namespace WSPR_Utils
{
    void setup()
    {
        Wire.setPins(18, 19);
        Wire.begin();

        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0)
        {
            return;
        }

        Serial.println("Si5351 not detected at adress 96");

        address = 98;

        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0)
        {
            return;
        }

        Serial.println("Si5351 not Detected at adress 98. There is no Si5351!");

        while (1)
        {
        }
    }

    void setRegister(uint8_t reg, uint8_t data)
    {
        Wire.beginTransmission(address);
        Wire.write(reg);
        Wire.write(data);
        Wire.endTransmission();
    }

    void disableTX(uint8_t clk)
    {
        setRegister(clk, 0x80);
    }

    uint8_t calculatePower(uint8_t dBmIn)
    {
        const uint8_t valid_dbm[19] =
            {0, 3, 7, 10, 13, 17, 20, 23, 27, 30, 33, 37, 40,
             43, 47, 50, 53, 57, 60};

        uint8_t validateddBmValue = dBmIn;
        if (validateddBmValue > 60)
            validateddBmValue = 60;

        for (uint8_t i = 0; i < 19; i++)
        {
            if (dBmIn >= valid_dbm[i])
                validateddBmValue = valid_dbm[i];
        }

        return validateddBmValue;
    }

    void convolve(uint8_t *c, uint8_t *s, uint8_t message_size, uint8_t bit_size)
    {
        uint32_t reg_0 = 0;
        uint32_t reg_1 = 0;
        uint32_t reg_temp = 0;
        uint8_t input_bit, parity_bit;
        uint8_t bit_count = 0;
        uint8_t i, j, k;

        for (i = 0; i < message_size; i++)
        {
            for (j = 0; j < 8; j++)
            {
                input_bit = (((c[i] << j) & 0x80) == 0x80) ? 1 : 0;

                reg_0 = reg_0 << 1;
                reg_1 = reg_1 << 1;
                reg_0 |= (uint32_t)input_bit;
                reg_1 |= (uint32_t)input_bit;

                reg_temp = reg_0 & 0xf2d05351;
                parity_bit = 0;
                for (k = 0; k < 32; k++)
                {
                    parity_bit = parity_bit ^ (reg_temp & 0x01);
                    reg_temp = reg_temp >> 1;
                }
                s[bit_count] = parity_bit;
                bit_count++;

                reg_temp = reg_1 & 0xe4613c47;
                parity_bit = 0;
                for (k = 0; k < 32; k++)
                {
                    parity_bit = parity_bit ^ (reg_temp & 0x01);
                    reg_temp = reg_temp >> 1;
                }
                s[bit_count] = parity_bit;
                bit_count++;
                if (bit_count >= bit_size)
                    break;
            }
        }
    }

    void wspr_interleave(uint8_t *s)
    {
        uint8_t d[WSPR_SYMBOL_COUNT];
        uint8_t rev, index_temp, i, j, k;

        i = 0;

        for (j = 0; j < 255; j++)
        {
            index_temp = j;
            rev = 0;

            for (k = 0; k < 8; k++)
            {
                if (index_temp & 0x01)
                {
                    rev = rev | (1 << (7 - k));
                }
                index_temp = index_temp >> 1;
            }

            if (rev < WSPR_SYMBOL_COUNT)
            {
                d[rev] = s[i];
                i++;
            }

            if (i >= WSPR_SYMBOL_COUNT)
            {
                break;
            }
        }

        memcpy(s, d, WSPR_SYMBOL_COUNT);
    }

    void wspr_merge_sync_vector(uint8_t *g)
    {
        uint8_t i;
        const uint8_t sync_vector[WSPR_SYMBOL_COUNT] =
            {1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0,
             1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0,
             0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1,
             0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0,
             1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1,
             0, 0, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1,
             1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
             1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0};

        for (i = 0; i < WSPR_SYMBOL_COUNT; i++)
        {
            txBuffer[i] = sync_vector[i] + (2 * g[i]);
        }
    }

    uint8_t wspr_code(char c)
    {
        if (isdigit(c))
        {
            return (uint8_t)(c - 48);
        }
        else if (c == ' ')
        {
            return 36;
        }
        else if (c >= 'A' && c <= 'Z')
        {
            return (uint8_t)(c - 55);
        }
        else
        {
            return 255;
        }
    }

    char letterize(int x)
    {
        return (char)x + 65;
    }

    char *get_mh(double lat, double lon)
    {
        int size = 4;
        static char locator[5];
        double LON_F[] = {20, 2.0, 0.083333, 0.008333, 0.0003472083333333333};
        double LAT_F[] = {10, 1.0, 0.0416665, 0.004166, 0.0001735833333333333};
        int i;
        lon += 180;
        lat += 90;

        size /= 2;
        size *= 2;

        for (i = 0; i < size / 2; i++)
        {
            if (i % 2 == 1)
            {
                locator[i * 2] = (char)(lon / LON_F[i] + '0');
                locator[i * 2 + 1] = (char)(lat / LAT_F[i] + '0');
            }
            else
            {
                locator[i * 2] = letterize((int)(lon / LON_F[i]));
                locator[i * 2 + 1] = letterize((int)(lat / LAT_F[i]));
            }
            lon = fmod(lon, LON_F[i]);
            lat = fmod(lat, LAT_F[i]);
        }
        locator[i * 2] = 0;
        return locator;
    }

    void wspr_encode(uint8_t power)
    {
        memset(txBuffer, 0, sizeof(txBuffer));

        char callsign[7] = {' ', ' ', ' ', ' ', ' ', ' ', 0};

        for (int i = 0; i < 6; i++)
            callsign[i] = Config.callsign.charAt(i);

        char *locator = get_mh(Config.beacon.latitude, Config.beacon.longitude);

        uint32_t n = wspr_code(callsign[0]);
        n = n * 36 + wspr_code(callsign[1]);
        n = n * 10 + wspr_code(callsign[2]);
        n = n * 27 + (wspr_code(callsign[3]) - 10);
        n = n * 27 + (wspr_code(callsign[4]) - 10);
        n = n * 27 + (wspr_code(callsign[5]) - 10);

        uint32_t m = ((179 - 10 * (locator[0] - 'A') - (locator[2] - '0')) * 180) + (10 * (locator[1] - 'A')) + (locator[3] - '0');
        m = (m * 128) + power + 64;

        uint8_t c[11];

        c[3] = (uint8_t)((n & 0x0f) << 4);
        n = n >> 4;
        c[2] = (uint8_t)(n & 0xff);
        n = n >> 8;
        c[1] = (uint8_t)(n & 0xff);
        n = n >> 8;
        c[0] = (uint8_t)(n & 0xff);

        c[6] = (uint8_t)((m & 0x03) << 6);
        m = m >> 2;
        c[5] = (uint8_t)(m & 0xff);
        m = m >> 8;
        c[4] = (uint8_t)(m & 0xff);
        m = m >> 8;
        c[3] |= (uint8_t)(m & 0x0f);
        c[7] = 0;
        c[8] = 0;
        c[9] = 0;
        c[10] = 0;

        uint8_t s[WSPR_SYMBOL_COUNT];
        convolve(c, s, 11, WSPR_SYMBOL_COUNT);

        wspr_interleave(s);

        wspr_merge_sync_vector(s);
    }

    String uint64ToStr(uint64_t p_InNumber, boolean p_LeadingZeros)
    {
        char l_HighBuffer[7];
        char l_LowBuffer[7];
        char l_ResultBuffer[13];
        String l_ResultString = "";
        uint8_t l_Digit;

        sprintf(l_HighBuffer, "%06lu", p_InNumber / 1000000L);
        sprintf(l_LowBuffer, "%06lu", p_InNumber % 1000000L);
        l_ResultString = l_HighBuffer;
        l_ResultString = l_ResultString + l_LowBuffer;

        if (!p_LeadingZeros)
        {
            l_ResultString.toCharArray(l_ResultBuffer, 13);
            for (l_Digit = 0; l_Digit < 12; l_Digit++)
            {
                if (l_ResultBuffer[l_Digit] == '0')
                {
                    l_ResultBuffer[l_Digit] = ' ';
                }
                else
                {
                    break;
                }
            }
            l_ResultString = l_ResultBuffer;
            l_ResultString.trim();
        }
        return l_ResultString;
    }

    void setupPLL(uint8_t pll, uint8_t mult, uint32_t num, uint32_t denom)
    {
        uint32_t P1;
        uint32_t P2;
        uint32_t P3;

        P1 = (uint32_t)(128 * ((float)num / (float)denom));
        P1 = (uint32_t)(128 * (uint32_t)(mult) + P1 - 512);
        P2 = (uint32_t)(128 * ((float)num / (float)denom));
        P2 = (uint32_t)(128 * num - denom * P2);
        P3 = denom;

        setRegister(pll + 0, (P3 & 0x0000FF00) >> 8);
        setRegister(pll + 1, (P3 & 0x000000FF));
        setRegister(pll + 2, (P1 & 0x00030000) >> 16);
        setRegister(pll + 3, (P1 & 0x0000FF00) >> 8);
        setRegister(pll + 4, (P1 & 0x000000FF));
        setRegister(pll + 5, ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
        setRegister(pll + 6, (P2 & 0x0000FF00) >> 8);
        setRegister(pll + 7, (P2 & 0x000000FF));
    }

    void setupMultisynth(uint8_t synth, uint32_t Divider, uint8_t rDiv)
    {
        uint32_t P1;
        uint32_t P2;
        uint32_t P3;

        P1 = 128 * Divider - 512;
        P2 = 0;
        P3 = 1;

        setRegister(synth + 0, (P3 & 0x0000FF00) >> 8);
        setRegister(synth + 1, (P3 & 0x000000FF));
        setRegister(synth + 2, ((P1 & 0x00030000) >> 16) | rDiv);
        setRegister(synth + 3, (P1 & 0x0000FF00) >> 8);
        setRegister(synth + 4, (P1 & 0x000000FF));
        setRegister(synth + 5, ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
        setRegister(synth + 6, (P2 & 0x0000FF00) >> 8);
        setRegister(synth + 7, (P2 & 0x000000FF));
    }

    void setFrequency(uint64_t frequency)
    {
        uint32_t ReferenceFrequency = 26000000;
        uint8_t OutputDivider = 1;
        uint8_t rDiv = SI_R_DIV_1;

        if (frequency < 100000000ULL)
        {
            OutputDivider = 128;
            rDiv = SI_R_DIV_128;
        }

        uint32_t Divider = 90000000000ULL / (frequency * OutputDivider);

        uint64_t pllFreq = Divider * frequency * OutputDivider;
        uint8_t mult = pllFreq / (ReferenceFrequency * 100UL);

        uint32_t l = pllFreq % (ReferenceFrequency * 100UL);
        float f = l;
        f *= 1048575;
        f /= ReferenceFrequency;
        uint32_t num = f;
        uint32_t denom = 1048575;
        num = num / 100;

        setupPLL(SI_SYNTH_PLL_A, mult, num, denom);

        setupMultisynth(SI_SYNTH_MS_0, Divider, rDiv);

        int32_t freqChange = frequency;

        if (abs(freqChange) > 100000)
        {
            setRegister(SI_PLL_RESET, 0xA0);
        }

        setRegister(SI_CLK0_CONTROL, 0x4F | SI_CLK_SRC_PLL_A);

        Serial.print(F("{TFQ} "));
        Serial.println(uint64ToStr(frequency, false));
    }

    void prepareWSPR(uint32_t altitude)
    {
        power1 = calculatePower(altitude / 300);
        power2 = calculatePower((altitude - (power1 * 300)) / 20);
    }

    void TX(uint64_t freq)
    {
        unsigned long startedAt = millis();
        for (uint8_t i = 0; i < 162; i++)
        {
            unsigned long stopAt = startedAt + ((i + 1) * (unsigned long)683);
            uint64_t toneFreq = freq + ((txBuffer[i] * 146));

            setFrequency(toneFreq);

            while ((millis() < stopAt))
                ;
            {
            }
        }

        disableTX(SI_CLK0_CONTROL);
    }

    void sendWSPR()
    {
        Utils::println("---> WSPR TX START");

        uint64_t freq = WSPR_FREQ20m + (100ULL * random(-100, 100));

        disableTX(SI_CLK0_CONTROL);

        wspr_encode(power1);
        TX(freq);

        Utils::println("First WSPR TX done, starting second one");

        delay(9000);

        wspr_encode(power2);
        TX(freq);

        Utils::println("---> WSPR TX DONE");
    }

    bool isInTimeslot(int minute, int second)
    {
        minute++;

        if (minute == 10 || minute == 30 || minute == 50)
        {
            if (second > 30)
            {
                return true;
            }
        }

        return false;
    }

}