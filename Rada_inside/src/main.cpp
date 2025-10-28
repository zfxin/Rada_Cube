// #include "main.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include "main.h"
#include "../lib/pinconfig.h"


RTC_DATA_ATTR int bootCount = 0;

void setup()
{

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(WAKE_PIN, INPUT);
  buzzer_init();
  ++bootCount;
  printf("Boot count: %d\r\n", bootCount);

  SYSTEM_MODE system_mode = NOMAL_MODE;

// ---------------------------- 检测按键唤醒原因 ---------------------------------
  // esp_sleep_wakeup_cause_t wakeup_reason;
  // wakeup_reason = esp_sleep_get_wakeup_cause();
  // switch(wakeup_reason)
  // {
  //   case ESP_SLEEP_WAKEUP_GPIO: printf("wake caused by RTC_IO"); break;
  //   default : printf("Wakeup was not caused by deep sleep: %d\r\n",wakeup_reason); break;
  // }
// ------------------------------------------------------------------------------

// -------------------------- 检测按键状态 ---------------------------------
  bool user_button_level = digitalRead(WAKE_PIN);
  
// 按键误触检测
  for(int i = 0; i < 3; i++)
  {
    user_button_level &= digitalRead(WAKE_PIN);
    delay(100);
    printf("user_button_level: %d\r\n", user_button_level);
  }

  if(user_button_level == LOW)
  {
    uint32_t user_button_start_time = millis();
    uint32_t user_button_end_time = 0;
    bool user_button_released = false;
    
    // 按键按下点亮LED，直到按键松开，指示按键按下
    digitalWrite(LED_PIN, HIGH);
    while(user_button_level == LOW)
    {
      user_button_level = digitalRead(WAKE_PIN);
      if(user_button_level == HIGH && user_button_released == false)
      {
        user_button_end_time = millis();
        user_button_released = true;
      }
      delay(300);
    }
    uint32_t user_button_press_time = 0;
    if(user_button_end_time == 0)
    {
      user_button_end_time = millis();
    }
    user_button_press_time = user_button_end_time - user_button_start_time;
    printf("User Button pressed time: %lu\r\n", user_button_press_time);

    digitalWrite(LED_PIN, LOW);
    // ---------------------------- 根据按键按下的不同时间进入不同的模式 ---------------------------------
    if(user_button_press_time > 10000)
    {
      system_mode = RECONFIG_MODE;
    }
    else if(user_button_press_time > 3000)
    {
      system_mode = TEST_MODE;
    }
    else if(user_button_press_time > 1000)
    {
      system_mode = WORK_MODE;
    }
    else
    {
      system_mode = NOMAL_MODE;
    }
  }
  printf("System mode: %d\r\n", system_mode);

  // ----------------------------根据系统模式执行对应的操作---------------------------------
  switch(system_mode)
  {
    case NOMAL_MODE:
      printf("Sysytem mode: NOMAL_MODE\r\n");
      led_blink(1);
      break;
    case WORK_MODE:
      printf("Sysytem mode: WORK_MODE\r\n");
      led_blink(2);
      break;
    case TEST_MODE:
      printf("Sysytem mode: TEST_MODE\r\n");
      led_blink(3);
      test_mode();
      break;
    case RECONFIG_MODE:
      printf("Sysytem mode: RECONFIG_MODE\r\n");
      led_blink(4);
      break;
  }
  digitalWrite(LED_PIN, LOW);

  esp_deep_sleep_enable_gpio_wakeup(BIT(WAKE_PIN), ESP_GPIO_WAKEUP_GPIO_LOW);
  printf("Going to sleep now\r\n");
  esp_deep_sleep_start();

}


void loop()
{
  
}


