#include "app.h"

// #include <vector>

#include "../system/dev/stm32_sleep.h"
#include "logging.h"
#include "utils.h"
#include "../system/dev/watchdog.h"
#ifdef MQTT
#include "comm/mqttcomm.h"
#else
#include "comm/httpcomm.h"
#endif

DigitalOut runLed(LED1);
AnalogIn   ain(A1);

#define TIM_USR      TIM3
#define TIM_USR_IRQ  TIM3_IRQn

TIM_HandleTypeDef mTimUserHandle;
extern "C" {
void M_TIM_USR_Handler(void);
}

#define GPSFIX_TIMEOUT_S  10

// #define DEBUG_NOGPS

namespace tag12 {

typedef enum
{
  NDEF = 0,
  SLEEP, 
  CHECKINGMOTION
} CHECK_STATES;

const conn_cfg apns[] = { {APN_VGESACE_NB_IOT, GSM_USNAME, GSM_PWD, NETWORK_CODE, NBIOT_BAND, true}, 
                          {APN_DEVAPN_GDSP, GSM_USNAME, GSM_PWD, NETWORK_CODE, 0, false}};

// const conn_cfg apns[] = {{APN_DEVAPN_GDSP, GSM_USNAME, GSM_PWD, NETWORK_CODE, 0, false}};
// const conn_cfg apns[] = {{APN_VGESACE_NB_IOT, GSM_USNAME, GSM_PWD, NETWORK_CODE, NBIOT_BAND, true}};

const int apns_len = sizeof(apns)/sizeof(conn_cfg);

App::App() : bg96if_(D8, D2, false), config_(&stm32sleep_, cloud_comm_), app_state_(&stm32sleep_) {
#ifdef MQTT
  cloud_comm_ = new MQTTComm(&bg96if_);
#else
  cloud_comm_ = new HTTPComm(&bg96if_);
#endif
  config_.SetCloudComm(cloud_comm_);
}

bool App::AccInit() {
  ext_i2c_ = new DevI2C(D14, D15);
  if (ext_i2c_ == NULL) {
    LOG("I2C init False");
    return false;
  }
  acc_sensor_ = new LSM6DSLSensor(ext_i2c_, LSM6DSL_ACC_GYRO_I2C_ADDRESS_HIGH, D4, D5);
  if (acc_sensor_ == NULL) {
    LOG("ACC init False");
    return false;
  }

  acc_sensor_->init(NULL);
  acc_sensor_->enable_x();
  acc_sensor_->enable_temp();
  acc_sensor_->disable_tilt_detection();
  acc_sensor_->disable_free_fall_detection();

  return true;
}

void App::AccDeinit() {
  if (acc_sensor_ == NULL)
    return;

  acc_sensor_->disable_g();
  acc_sensor_->disable_x();
}

void App::AccEnableInt() {
  if (acc_sensor_ == NULL)
    return;

  if (acc_sensor_->enable_wake_up_detection(LSM6DSL_INT1_PIN) != 0) {
    LOG("Set wakeup detection failed");
  }
  acc_sensor_->set_wake_up_threshold(config_.GetConfig().bits.acc_threshold);
  acc_sensor_->set_x_odr(26.0f);
  unsigned char tmp;
  acc_sensor_->read_reg(0x15, &tmp);
  tmp |= (1<<4);
  acc_sensor_->write_reg(0x15, tmp);
}

void App::AccDisableInt() {
  if (acc_sensor_ == NULL)
    return;

  acc_sensor_->disable_wake_up_detection();
  acc_sensor_->disable_tilt_detection();
  acc_sensor_->disable_free_fall_detection();
}

GpsData App::GetGps(int fix_timeout) {
  char gps_raw[128] = "+QGPSLOC: 122500.0,47.74811,9.58924,1.3,420.0,2,36.03,0.0,0.0,010319,09";
  GpsData gps_res;
  gps_res.valid = false;
  gps_res.timestamp = 0;
  gps_res.lon = 0;
  gps_res.lat = 0;
  gps_res.speed = -1.0f;

#ifdef DEBUG_NOGPS
  return gps_res;
#endif
  int max_attempt = (fix_timeout / GPSFIX_TIMEOUT_S);
  if (max_attempt == 0)
    max_attempt = 1;

  while (!gps_res.valid && (max_attempt-- > 0)) {
#ifdef DEBUG_GPS
    wait(20);
    {
#else
    memset(gps_raw, 0, sizeof(gps_raw));
    if (bg96if_.getGPS(gps_raw, sizeof(gps_raw))) {
#endif
      char gpstime[7];
      char gpsdate[7];
      char* chars_array = strtok(gps_raw, ":,");
      int pos = 0;
      while ((chars_array) && (pos < 11)) {
        switch(pos++) {
          case 1: strcpy(gpstime, chars_array); break;
          case 2: gps_res.lat = atof(chars_array);break;
          case 3: gps_res.lon = atof(chars_array); break;          
          case 8: gps_res.speed = atof(chars_array); break;
          case 10: strcpy(gpsdate, chars_array); break;
        }
        chars_array = strtok(NULL, ":,");
      }
      gps_res.valid = true;
      gps_res.timestamp = Utils::ConvertGPSTime(gpstime, gpsdate);
    }

    if ((!gps_res.valid) && (max_attempt > 0))
      wait(GPSFIX_TIMEOUT_S);
  } 

  return gps_res;
}

uint32_t App::GetCell() {
  char res[64];

  memset(res, 0, sizeof(res));
  if (!bg96if_.creg(res, sizeof(res))) {
    LOG("Error Creg");
    return 0;
  }

  char* chars_array = strtok(res, ":,");
  int pos = 0;
  uint32_t cell_id = 0;
  while ((chars_array) && (pos < 5)) {
    if (pos == 4) {
      sscanf(chars_array, "\"%x\"", &cell_id);
    }
    
    chars_array = strtok(NULL, ":,");
    pos++;
  }

  return cell_id;
}

bool App::ConnectGsm(conn_cfg connection_cfg) {
  LOG("Connecting ... ");

  LOG("Trying connection to APN: %s", connection_cfg.apn);
  if (bg96if_.setConnection(connection_cfg.band, connection_cfg.code, 
        connection_cfg.apn, connection_cfg.usr, connection_cfg.pwd, connection_cfg.nbiot)) {
    LOG("Connection to APN OK!: %s", connection_cfg.apn);
    return bg96if_.connect();
  }

  LOG("Connection failed to APN: %s", connection_cfg.apn);
  return false;
}

float App::GetVoltage() {
  float voltage = 0.0f;
  int max_attempt = 5;
  while ((voltage < 0.1f) && (max_attempt-- > 0)) {
    wait_ms(100);
    voltage = (3.3f / 65535.0f) * ain.read_u16() * 2;
  }

  return voltage;
}

float App::GetTemperature() {
  if (acc_sensor_ == NULL)
    return 0.0;

  float res = acc_sensor_->read_temperatureC();
  LOG("Temperature: %lf", res);
  return res;
}
/*
bool App::ConnectMqtt() {
  int connack_rc;
  int retry_attempt = 0;
  while ((connack_rc = cloud_comm_->Connect())
       != MQTT_CONNECTION_ACCEPTED) {    
    if (connack_rc == MQTT_NOT_AUTHORIZED || connack_rc == MQTT_BAD_USERNAME_OR_PASSWORD) {
      // printf ("File: %s, Line: %d Error: %d\n\r",__FILE__,__LINE__, connack_rc);        
      return false; // don't reattempt to connect if credentials are wrong
    } 
    
    int timeout = (retry_attempt < 10) ? 3 : (retry_attempt < 20) ? 60 : 600;
    if (++retry_attempt == 5) {
      // pc.printf ("\n\n\rFAIL!! system reset!!\n\n\r");
      return false;
    }
    else
      wait(timeout);
  }
  
}
*/

bool App::SendData(GpsData gps_data_valid, const char * mac_addr, bool nbiot, bool gps_init_err) {
  DataSend data;
  data.log_err.intValue = 0;
  data.timestamp = gps_data_valid.timestamp;
  data.gps_data = gps_data_valid;
  data.mac_address = const_cast<char*>(mac_addr);
  data.voltage = GetVoltage();
  data.cell_id = GetCell();
  data.temperature = GetTemperature();
  data.nbiot = static_cast<bool>(nbiot);
  data.log_err.bits.err_sent = static_cast<int>(app_state_.GetAppStateData().bits.log_err_sent);
  data.log_err.bits.gps_init_err = static_cast<int>(gps_init_err);

  return cloud_comm_->Publish(data);
}

void App::SyncTime(uint32_t gpstime) {
  uint32_t rtctime = stm32sleep_.GetTime();
  if (abs(gpstime - rtctime) > 2) {
    LOG("Sync time");
// #ifndef DEBUG_NOGPS
    // stm32sleep_.SetTime(gpstime);
// #endif
  }
}

void App::TimerInit() {
  __HAL_RCC_TIM3_CLK_ENABLE();
  mTimUserHandle.Instance               = TIM_USR;
  mTimUserHandle.Init.Prescaler         = 47999;
  mTimUserHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
  mTimUserHandle.Init.Period            = 60000; // 1 min interval at 48 MHz MCU speed
  mTimUserHandle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  HAL_TIM_Base_Init(&mTimUserHandle);
  HAL_TIM_Base_Start_IT(&mTimUserHandle);
 
  NVIC_SetVector(TIM_USR_IRQ, (uint32_t)M_TIM_USR_Handler);
  NVIC_EnableIRQ(TIM_USR_IRQ);
}

void App::TimerDeinit() {
  NVIC_DisableIRQ(TIM_USR_IRQ);
}

#ifdef DEBUG
void App::TestAcc() {
  int32_t data[3];

  AccInit();
  while (acc_sensor_->get_x_axes(data) == 0) {
    // print out
    LOG("Read acc x:%d y:%d z:%d", data[0], data[1], data[2]);
    wait(0.1);
  }
}
#endif

#ifdef DEBUG
void App::RunTest() {
  LOG("TEST -------------------");
  runLed = 1;
  bool activate_acc = true;
  STM32Sleep stm32sleep;
  stm32sleep.InitSleep();
  
  AccInit();
  AccDisableInt();
  wait(0.1);

  /*time_t rtc_time = stm32sleep.GetTime();
  uint32_t lastsend_time = stm32sleep.rtc_read_backup_reg(LASTSEND_TIMESTAMP_REG);

  int state = stm32sleep.rtc_read_backup_reg(STATE_REG);
  bool deepsleep_wakeup = (abs(rtc_time - lastsend_time) >= (SEND_INTERVAL_MIN * 60));
  bool gps_data_check = true;*/
}
#endif

extern "C"
void M_TIM_USR_Handler(void) {
  volatile static uint32_t times = 0;
  if (__HAL_TIM_GET_FLAG(&mTimUserHandle, TIM_FLAG_UPDATE) == SET) {
    __HAL_TIM_CLEAR_FLAG(&mTimUserHandle, TIM_FLAG_UPDATE);
    times++;
    if (times > RESET_WD) {
      LOG("reset NVIC");
      NVIC_SystemReset();
    }
  }
}

void App::Run() {
  TimerInit();
  runLed = 1;
  bool activate_acc = true;
  stm32sleep_.InitSleep();
  int state = static_cast<int>(app_state_.GetAppStateData().bits.state);

  AccInit();
  AccDisableInt();

  wait(0.1);
  uint32_t lastsend_time = stm32sleep_.rtc_read_backup_reg(LASTSEND_TIMESTAMP_REG);
  uint32_t config_time = stm32sleep_.rtc_read_backup_reg(CONFIG_TIMESTAMP_REG);

  time_t rtc_time = stm32sleep_.GetTime();
  bool deepsleep_wakeup = (abs(rtc_time - lastsend_time) >= (config_.GetConfig().bits.send_interval_min * 60));
  bool config_update = ((abs(rtc_time - config_time) >= CONFIG_CHECK_S) || (config_time == 0));
  bool gps_data_send = true;

  LOG("Previous state 0x%x", state);
  LOG("rtcTime: %d last sendtime: %d", rtc_time, lastsend_time);

  if (deepsleep_wakeup) {
    LOG("Long sleep without sending");
    app_state_.SetLastWokeupAcc(false);
  } else {
    if (state == (int) SLEEP) {
      state = (int) CHECKINGMOTION;
      gps_data_send = false;
      if (app_state_.GetAppStateData().bits.last_wokeup_acc) {
        LOG("Woke up by ACC in previous state - rather send data");
        gps_data_send = true;
        app_state_.SetLastWokeupAcc(false);
        state = (int) SLEEP;
      }
    } else if (state == (int) CHECKINGMOTION) {
      uint32_t inmotion_check_time = stm32sleep_.rtc_read_backup_reg(INMOTION_CHECK_TIME);
      bool wokenup = (abs(rtc_time - inmotion_check_time) < (config_.GetConfig().bits.wakeup_time * 60));
      if (wokenup) {
        LOG("Woke up by ACC");
        state = (int) SLEEP;
      } else {
        gps_data_send = false;
      }
      app_state_.SetLastWokeupAcc(wokenup);
    } else {
      state = (int) CHECKINGMOTION;
    }
  }

  if (gps_data_send) {
    LOG("Waking up the BG96");
    bg96if_.wakeup();
    if (bg96if_.init()) {
      const char * mac_addr = bg96if_.get_mac_address();
      GpsData gps_res;
      gps_res.valid = false;

      LOG("Trying GPS");
      bool set_gps = false;
      if ((set_gps = bg96if_.setGPS())) {
        gps_res = GetGps(config_.GetConfig().bits.gps_fix_timeout);
        LOG("gps_res: timestamp: %d lon: %lf lat: %lf valid: %d", gps_res.timestamp, gps_res.lon, gps_res.lat, gps_res.valid);
      }
      
      if (gps_res.valid) {
        bg96if_.GPSOff();
      }
     
      bool sent = false;
      int apn_index = 0; 
      while (!sent && (apn_index < apns_len)) {
        if (ConnectGsm(apns[apn_index])) {
          if (config_update) {
            config_.UpdateConfig(mac_addr);
            stm32sleep_.rtc_write_backup_reg(CONFIG_TIMESTAMP_REG, stm32sleep_.GetTime());
          }
          if (!gps_res.valid) {
            LOG("Check the GPS once more");
            gps_res = GetGps(0);
            LOG("gps_res: timestamp: %d lon: %lf lat: %lf valid: %d", gps_res.timestamp, gps_res.lon, gps_res.lat, gps_res.valid);
          }
          LOG("Sending data ... ");
          sent = SendData(gps_res, mac_addr, apns[apn_index].nbiot, !set_gps);
        }
        apn_index++;
      }

      if (sent) {
        stm32sleep_.rtc_write_backup_reg(LASTSEND_TIMESTAMP_REG, stm32sleep_.GetTime());
      }
      app_state_.SetLogErrSent(!sent);
    }

    bg96if_.GPSOff();
  }

  LOG("Current state 0x%x", state);
  stm32sleep_.SetAlarm(config_.GetConfig().bits.wakeup_time * 60);
  if (state == (int) SLEEP) {
    LOG("Deinit of ACC");
    LOG("sleep - wakeup in %d s", (config_.GetConfig().bits.wakeup_time * 60));
    AccDeinit();
  } else if (state == (int) CHECKINGMOTION) {
    LOG("Init of ACC interrupt");
    LOG("sleep - wakeup in %d s or ACC", (config_.GetConfig().bits.wakeup_time * 60));
    stm32sleep_.rtc_write_backup_reg(INMOTION_CHECK_TIME, rtc_time);
    AccEnableInt();
  }

  TimerDeinit();
  LOG("Shutdown BG96");
  bg96if_.shutdown();
  app_state_.SetState(state);
  LOG("Go to standby");
  wait(1);
  stm32sleep_.StandBy();
}

} // namespace tag12