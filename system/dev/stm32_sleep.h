#ifndef STM32_SLEEP
#define STM32_SLEEP

#include "mbed.h"
#include "rtc_api.h"
#include "sleep_api.h"
#include "stm32f1xx.h"

typedef enum
{
  REG1 = 1, 
  REG2,
  REG3,
  REG4,
  REG5
} BACKUP_REG;

class STM32Sleep {
 public:
  STM32Sleep();
  void InitSleep();
  time_t GetTime();
  void SetTime(time_t seconds_epoch);
  bool SetAlarm(uint32_t seconds);
  void StandBy();
  uint32_t rtc_read_backup_reg(BACKUP_REG reg);
  bool rtc_read_backup_reg_bit(BACKUP_REG reg, uint8_t bit);
  void rtc_write_backup_reg(BACKUP_REG reg, uint32_t data);
  void rtc_write_backup_reg_bit(BACKUP_REG reg, uint8_t bit, bool value);

 private:
  uint32_t backup_reg_[5];
  bool backup_reg_changed_[5];
};

#endif