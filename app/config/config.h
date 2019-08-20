#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "Json.h"

#include "../../system/dev/stm32_sleep.h"
#include "../comm/cloudcomm.h"
#include "config_default.h"

namespace tag12 {

typedef union config_data {
  struct {
    unsigned wakeup_time : 6;  // 6 bits  minutes (1 - 63)    
    unsigned acc_threshold : 6;  // 6bits (1 - 63)
    unsigned send_interval_min : 10; // 10 bits  (minutes)
    unsigned gps_fix_timeout : 9; // 9 bits (seconds)
    unsigned set : 1;  // the config is set (1)
  } bits;
  uint32_t intValue;
} ConfigData;

class Config {
 public:
  Config(STM32Sleep * const sleep, CloudComm * const cloud_comm);

  void SetCloudComm(CloudComm * const cloud_comm) { cloud_comm_ = cloud_comm; }
  ConfigData GetConfig();
  void SetDefault();
  void UpdateConfig(const char * mac);
 private:
  void GetJsonValue(Json * const json, char * item, char * data);
  void StoreConfig(const ConfigData & config);
  STM32Sleep * const sleep_;
  CloudComm * cloud_comm_;

};

} // namespace tag12

#endif