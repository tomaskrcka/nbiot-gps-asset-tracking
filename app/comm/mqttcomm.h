#ifdef false

#ifndef APP_MQTT_H
#define APP_MQTT_H

#include "mbed.h"
#include "MQTTClient.h"
#include "MQTTmbed.h"
#include "CellularInterface.h"
#include "MQTTSocket.h"

#include "cloudcomm.h"
#include "../config/config_default.h"

#define MQTT_MAX_PACKET_SIZE 300

namespace  tag12 {

class MQTTComm : public CloudComm {
 public:
  MQTTComm(NetworkStack* const ipstack);
  virtual bool Publish(DataSend data);
  virtual int Connect();
  virtual bool Disconnect();
  virtual bool GetConfig(const char * mac, char * data, int size) { return false; }
 private:
  MQTTSocket mqtt_socket_;
  MQTT::Client<MQTTSocket, Countdown, MQTT_MAX_PACKET_SIZE> client_;
  NetworkStack* const ipstack_;
  bool connected_;
};

} // namespace tag12

#endif

#endif