#include "config.h"

#include "../../logging.h"

namespace tag12 {

Config::Config(STM32Sleep * const sleep, CloudComm * const cloud_comm) : sleep_(sleep), cloud_comm_(cloud_comm) {

}

ConfigData Config::GetConfig() {
  uint32_t config_res = sleep_->rtc_read_backup_reg(CONFIG_BITS);
  ConfigData config_data;
  config_data.intValue = config_res;

  if (config_data.bits.set == 0) {
    LOG("Config was not set");
    config_data.bits.acc_threshold = ACC_THRESHOLD_DEFAULT;
    config_data.bits.gps_fix_timeout = GPS_FIX_TIMEOUT_DEFAULT;
    config_data.bits.send_interval_min = SEND_INTERVAL_MIN_DEFAULT;
    config_data.bits.wakeup_time = WAKEUP_INTERVAL_MIN_DEFAULT;
    config_data.bits.set = 1;
    StoreConfig(config_data);
  }

  return config_data;
}


void Config::GetJsonValue(Json * const json, char * item, char * data) {
  char tmpValue[16];
  
  int index = json->findKeyIndexIn(item, 0);
  if ( index == -1) {
    LOG("\"%s\" does not exist", item);
    data[0] = 0;
  } else {
    int valueIndex = json->findChildIndexOf(index, -1);
    if (valueIndex > 0 ) {
      const char * valueStart  = json->tokenAddress(valueIndex);
      int valueLength = json->tokenLength(valueIndex);
      strncpy(data, valueStart, valueLength);
      data[valueLength] = 0; // NULL-terminate the string
      
      LOG("%s: %s", item, data);
    }
  }
}

void Config::UpdateConfig(const char * mac) {
  char buf[256];
  if (cloud_comm_ == NULL) {
    LOG("cloudcomm is not set");
    return;
  }
  
  cloud_comm_->GetConfig(mac, buf, sizeof(buf));
  LOG("Config: %s", buf);
  ConfigData config_data;
  int acc_threshold = 0;
  int gps_fix_timeout = 0;
  int send_interval_min = 0;
  int wakeup_time = 0;

  /*sscanf(buf, "{\"gpsFixTimeout\":%d,\"accThreshold\":%lf,\"wakeupTime\":%d,\"interruptTime\":%d}",
    &gps_fix_timeout, &acc_threshold, &wakeup_time, &send_interval_min);*/
  
  Json json (buf, strlen(buf));

  if ( !json.isValidJson () )
  {
    LOG( "Invalid JSON: %s", buf);
    return;
  }

  if (json.type(0) != JSMN_OBJECT)
  {
      LOG( "Invalid JSON.  ROOT element is not Object: %s", buf);
      return;
  }
  
  char tmp_value[16];

  GetJsonValue(&json, "accThreshold", tmp_value);
  if (tmp_value[0] != 0) {
    acc_threshold = atoi(tmp_value);
  } else {
    LOG("No accThreshold, ending ..");
    return;
  }

  if (acc_threshold > 0) {
    config_data.bits.acc_threshold = acc_threshold;
  } else {
    LOG("No valid accThreshold, ending ..");
    return;
  }
  
  GetJsonValue(&json, "gpsFixTimeout", tmp_value);
  if (tmp_value[0] != 0) {
    gps_fix_timeout = atoi(tmp_value);
  } else {
    LOG("No gpsFixTimeout, ending ..");
    return;
  }

  if (gps_fix_timeout > 0) {
    config_data.bits.gps_fix_timeout = gps_fix_timeout;
  } else {
    LOG("No valid gpsFixTimeout, ending ..");
    return;
  }
  
  GetJsonValue(&json, "interruptTime", tmp_value);
  if (tmp_value[0] != 0) {
    send_interval_min = atoi(tmp_value);
  } else {
    LOG("No interruptTime, ending ..");
    return;
  }

  if (send_interval_min > 0) {
    config_data.bits.send_interval_min = send_interval_min;
  } else {
    LOG("No valid interruptTime, ending ..");
    return;
  }
    
  GetJsonValue(&json, "wakeupTime", tmp_value);
  if (tmp_value[0] != 0) {
    wakeup_time = atoi(tmp_value);
  } else {
    LOG("No wakeupTime, ending ..");
    return;
  }

  if (wakeup_time > 0) {
    config_data.bits.wakeup_time = wakeup_time;
  } else {
    LOG("No valid wakeupTime, ending ..");
    return;
  }

  LOG("Config set!");
  config_data.bits.set = 1;
  StoreConfig(config_data);
}

void Config::StoreConfig(const ConfigData & config) {
  sleep_->rtc_write_backup_reg(CONFIG_BITS, config.intValue);
}

void Config::SetDefault() {
  ConfigData config_data;
  config_data.bits.acc_threshold = ACC_THRESHOLD_DEFAULT;
  config_data.bits.gps_fix_timeout = GPS_FIX_TIMEOUT_DEFAULT;
  config_data.bits.send_interval_min = SEND_INTERVAL_MIN_DEFAULT;
  config_data.bits.wakeup_time = WAKEUP_INTERVAL_MIN_DEFAULT;
  config_data.bits.set = 1;

  StoreConfig(config_data);
}

} // namespace tag12