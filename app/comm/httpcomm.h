#ifndef APP_HTTPCOMM_H
#define APP_HTTPCOMM_H

#include "mbed.h"
#include "../system/dev/network/NbIotBG96/BG96Interface.h"
#include "cloudcomm.h"
#include "../config/config_default.h"

namespace  tag12 {

class HTTPComm : public CloudComm {
 public:
  HTTPComm(BG96Interface* const ipstack);
  virtual bool Publish(DataSend data);
  virtual bool GetConfig(const char * mac, char * data, int size);
  virtual int Connect();
  virtual bool Disconnect();
 private:
  BG96Interface* const ipstack_;
  bool connected_;
};

} // namespace tag12

#endif