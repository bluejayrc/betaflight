/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdint.h>

#include "rx/rx.h"
#include "io/rc_controls.h"
#include "io/escservo.h"

#include "io/rc_curves.h"

#include "config/config.h"

#define PITCH_LOOKUP_LENGTH 31
#define YAW_LOOKUP_LENGTH 31
#define THROTTLE_LOOKUP_LENGTH 12

static int16_t lookupPitchRollRC[PITCH_LOOKUP_LENGTH];      // lookup table for expo & RC rate PITCH+ROLL
static int16_t lookupYawRC[YAW_LOOKUP_LENGTH];              // lookup table for expo & RC rate YAW
static int16_t lookupThrottleRC[THROTTLE_LOOKUP_LENGTH];    // lookup table for expo & mid THROTTLE


void generatePitchRollCurve(controlRateConfig_t *controlRateConfig)
{
    uint8_t i;
    float j = 0;

    for (i = 0; i < PITCH_LOOKUP_LENGTH; i++) {
        lookupPitchRollRC[i] = (2500 + controlRateConfig->rcExpo8 * (j * j - 25)) * j * (int32_t) controlRateConfig->rcRate8 / 2500;
        j += 0.2f;
    }
}

void generateYawCurve(controlRateConfig_t *controlRateConfig)
{
    uint8_t i;
    float j = 0;

    for (i = 0; i < YAW_LOOKUP_LENGTH; i++) {
        lookupYawRC[i] = (2500 + controlRateConfig->rcYawExpo8 * (j * j - 25)) * j / 25;
        j += 0.2f;
    }
}

void generateThrottleCurve(controlRateConfig_t *controlRateConfig, escAndServoConfig_t *escAndServoConfig)
{
    uint8_t i;
    uint16_t minThrottle = (feature(FEATURE_3D && IS_RC_MODE_ACTIVE(BOX3DDISABLESWITCH)) ? PWM_RANGE_MIN : escAndServoConfig->minthrottle);

    for (i = 0; i < THROTTLE_LOOKUP_LENGTH; i++) {
        int16_t tmp = 10 * i - controlRateConfig->thrMid8;
        uint8_t y = 1;
        if (tmp > 0)
            y = 100 - controlRateConfig->thrMid8;
        if (tmp < 0)
            y = controlRateConfig->thrMid8;
        lookupThrottleRC[i] = 10 * controlRateConfig->thrMid8 + tmp * (100 - controlRateConfig->thrExpo8 + (int32_t) controlRateConfig->thrExpo8 * (tmp * tmp) / (y * y)) / 10;
        lookupThrottleRC[i] = minThrottle + (int32_t) (escAndServoConfig->maxthrottle - minThrottle) * lookupThrottleRC[i] / 1000; // [MINTHROTTLE;MAXTHROTTLE]
    }
}

int16_t rcLookupPitchRoll(int32_t tmp)
{
    const int32_t tmp2 = tmp / 20;
    return lookupPitchRollRC[tmp2] + (tmp - tmp2 * 20) * (lookupPitchRollRC[tmp2 + 1] - lookupPitchRollRC[tmp2]) / 20;
}

int16_t rcLookupYaw(int32_t tmp)
{
    const int32_t tmp2 = tmp / 20;
    return lookupYawRC[tmp2] + (tmp - tmp2 * 20) * (lookupYawRC[tmp2 + 1] - lookupYawRC[tmp2]) / 20;
}

int16_t rcLookupThrottle(int32_t tmp)
{
    const int32_t tmp2 = tmp / 100;
    // [0;1000] -> expo -> [MINTHROTTLE;MAXTHROTTLE]
    return lookupThrottleRC[tmp2] + (tmp - tmp2 * 100) * (lookupThrottleRC[tmp2 + 1] - lookupThrottleRC[tmp2]) / 100;
}

