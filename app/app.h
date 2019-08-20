#ifndef APP_APP_H
#define APP_APP_H

#include "mbed.h"
#include "../system/dev/network/NbIotBG96/BG96Interface.h"
#include "comm/cloudcomm.h"
#include "LSM6DSLSensor.h"

#include "config/config_default.h"
#include "config/config.h"
#include "app_state.h"

namespace tag12 {

typedef struct {
  char * apn;
  char * usr;
  char * pwd;
  char * code;
  uint32_t band;
  bool nbiot;
} conn_cfg;

class App {
 public:
  App();
  void Run();
#ifdef DEBUG
  void RunTest();
#endif

 private:
  void TimerInit();
  void TimerDeinit();
  bool AccInit();
  void AccDeinit();
  float GetVoltage();
  float GetTemperature();
  GpsData GetGps(int fix_timeout);
  uint32_t GetCell();
  void SyncTime(uint32_t gpstime);
  bool ConnectGsm(conn_cfg connection_cfg);
  bool SendData(GpsData gps_data_valid, const char * mac_addr, bool nbiot, bool gps_init_err = false);
  void AccEnableInt();
  void AccDisableInt();
#ifdef DEBUG
  void TestAcc();
#endif

  BG96Interface bg96if_;
  CloudComm * cloud_comm_;
  LSM6DSLSensor * acc_sensor_;
  DevI2C *ext_i2c_;
  STM32Sleep stm32sleep_;
  Config config_;
  AppState app_state_;
};

} // namespace tag12
#endif