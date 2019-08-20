#ifndef APP_CLOUDCOMM_H
#define APP_CLOUDCOMM_H

namespace tag12 {

struct GpsData {
  uint32_t timestamp;
  float lat;
  double lon;
  double speed;
  bool valid;
};

typedef union log_data {
  struct {
    unsigned err_sent : 1;  // 1 bit previous message lost
    unsigned gps_init_err : 1;  // 1 gps was not inicialized
    unsigned backup : 30; // just backup, not in use
  } bits;
  uint32_t intValue;
} LogData;

struct DataSend {
  char *mac_address;
  struct GpsData gps_data;
  
  uint32_t timestamp;
  /*
  double lat;
  double lon;
  uint8_t sat_count;
  uint8_t sat_tracked;
  double speed; */
  float voltage;
  int nbiot; // nbiot = 1
  uint32_t cell_id;
  LogData log_err;
  float temperature;
};

class CloudComm {
  public:
    virtual bool Publish(DataSend data) = 0;
    virtual bool GetConfig(const char * mac, char * data, int size) = 0;
    virtual int Connect() = 0;
    virtual bool Disconnect() = 0;
};

} // namespace tag12

#endif