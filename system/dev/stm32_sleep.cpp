#include "stm32_sleep.h"
#include "../../logging.h"

STM32Sleep::STM32Sleep() { 
  for (int i = 0; i < sizeof(backup_reg_changed_); i++) {
    backup_reg_changed_[i] = false;
  }
}

void STM32Sleep::InitSleep() {
  rtc_init();
  __HAL_RCC_BKP_CLK_ENABLE();
}

bool STM32Sleep::SetAlarm(uint32_t seconds) {
  RTC_AlarmTypeDef sAlarm;
  RTC_HandleTypeDef hrtc;

  hrtc.Instance = RTC;
  // rtc_write(0);
  time_t time = rtc_read();

  time += seconds;
  struct tm *timeinfo = localtime(&time);
  /*
  LOG("Hours: %d",  timeinfo->tm_hour);
  LOG("Minutes: %d",  timeinfo->tm_min);
  LOG("seconds: %d",  timeinfo->tm_sec);*/

  /* Enable the Alarm A */
  sAlarm.AlarmTime.Hours = timeinfo->tm_hour;
  sAlarm.AlarmTime.Minutes = timeinfo->tm_min;
  sAlarm.AlarmTime.Seconds = timeinfo->tm_sec;

  sAlarm.Alarm = RTC_ALARM_A;
  if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK) {
    return false;
  }

  return true;
}

time_t STM32Sleep::GetTime() { return rtc_read(); }

void STM32Sleep::SetTime(time_t seconds_epoch) { rtc_write(seconds_epoch); }

uint32_t STM32Sleep::rtc_read_backup_reg(BACKUP_REG reg) {
  if (backup_reg_changed_[reg - 1])
    return backup_reg_[reg - 1];

  RTC_HandleTypeDef RtcHandle;
  RtcHandle.Instance = RTC;
  uint32_t tmp = 0;
  uint8_t reg1 = ((int) reg*2 - 1);
  uint8_t reg2 = (int) reg*2;

  tmp = HAL_RTCEx_BKUPRead(&RtcHandle, reg2) << 16;
  tmp |= HAL_RTCEx_BKUPRead(&RtcHandle, reg1);

  return tmp;
}

bool STM32Sleep::rtc_read_backup_reg_bit(BACKUP_REG reg, uint8_t bit) {
  uint32_t reg_tmp = rtc_read_backup_reg(reg);
  if (((reg_tmp >> bit) & 0x1) == 1)
    return true;

  return false;
}

void STM32Sleep::rtc_write_backup_reg(BACKUP_REG reg, uint32_t data) {
  backup_reg_[reg - 1] = data;
  backup_reg_changed_[reg - 1] = true;
}

void STM32Sleep::rtc_write_backup_reg_bit(BACKUP_REG reg, uint8_t bit, bool value) {
  uint32_t reg_tmp = rtc_read_backup_reg(reg);
  if (value) {
    reg_tmp |= 0x1 << bit;
  } else {
    reg_tmp &= ~(0x1 << bit);
  }

  rtc_write_backup_reg(reg, reg_tmp);
}

void STM32Sleep::StandBy() {
  /* Enable Power Clock*/
  __HAL_RCC_PWR_CLK_ENABLE();

  /* Allow access to Backup */

  HAL_PWR_EnableBkUpAccess();
  RTC_HandleTypeDef RtcHandle;
  RtcHandle.Instance = RTC;

  for (int i = 0; i < sizeof(backup_reg_changed_); i++) {
    if (backup_reg_changed_[i]) {
      HAL_RTCEx_BKUPWrite(&RtcHandle, (i*2 + 2), (backup_reg_[i] >> 16));
      HAL_RTCEx_BKUPWrite(&RtcHandle, (i*2 + 1), (backup_reg_[i] & 0xffff));
    }
  }

  /* Reset RTC Domain */
  /*__HAL_RCC_BACKUPRESET_FORCE();
  __HAL_RCC_BACKUPRESET_RELEASE();*/

  /* Disable all used wake-up sources: Pin1(PA.0) */
  HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);

  /* Clear all related wake-up flags */
  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

  /* Re-enable all used wake-up sources: Pin1(PA.0) */
  HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
  //HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN5);

  /* Request to enter STANDBY mode  */
  HAL_PWR_EnterSTANDBYMode();
}