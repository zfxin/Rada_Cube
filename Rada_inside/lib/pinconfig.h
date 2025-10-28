#ifndef __PINCONFIG_H__
#define __PINCONFIG_H__

#include <Arduino.h>
#include <string>


#define LED_PIN              3
#define WAKE_PIN             5
#define BUZZER_PIN           4
#define BUZZER_FREQ          2700
#define BUZZER_RESOLUTION    8


inline constexpr std::string_view MATCH_SUCCESS_ACK = "MATCH_SUCCESS_ACK";
// inline constexpr const uint8_t* MATCH_SUCCESS_DATA = reinterpret_cast<const uint8_t*>(MATCH_SUCCESS_ACK.data());
inline constexpr std:: size_t MATCH_SUCCESS_SIZE = MATCH_SUCCESS_ACK.size();

enum SYSTEM_MODE
{
    NOMAL_MODE = 0,
    TEST_MODE,
    WORK_MODE,
    RECONFIG_MODE
};


#endif
