#ifdef false

#include "mqttcomm.h"
#include "../config/config_default.h"

#define MQTT_MAX_PAYLOAD_SIZE 500

namespace tag12 {

MQTTComm::MQTTComm(NetworkStack* const ipstack) : client_(mqtt_socket_),
    ipstack_(ipstack), connected_(false) {
  // Nothing
}

bool MQTTComm::Publish(DataSend data)
{
  char buf[MQTT_MAX_PAYLOAD_SIZE];
  int size = sprintf(buf,
      "{\"d\":{\"Timestamp\":%ld,\"lat\":%lf,\"lon\":%lf}}", data.timestamp, 
      data.lat, data.lon);

  MQTT::Message message;

  message.qos = MQTT::QOS0;
  message.retained = false;
  message.dup = false;
  message.payload = (void*)buf;
  message.payloadlen = size + 1;
  
  // pc.printf("Publishing V%s #%d %s\n\r", FW_REV, n_msg, buf);
  return ((client_.publish(MQTT_TOPIC, message) < 0) ? false : true);
}

int MQTTComm::Connect()
{ 
  mqtt_socket_.open(ipstack_);

  int rc = mqtt_socket_.connect(MQTT_BROKER_ADDR, MQTT_PORT, 1000);
  if (rc != 0)
  {  
    return rc;
  }

  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  data.MQTTVersion = 3;
  data.struct_version = 0;
  data.clientID.cstring = const_cast<char*>(MQTT_CLIENT_ID);

  if (strlen(MQTT_AUTH_TOKEN) > 0) {   
    data.username.cstring = const_cast<char*>("use-token-auth");
    data.password.cstring = const_cast<char*>(MQTT_AUTH_TOKEN);
  }

  rc = client_.connect(data);

  return rc;
}
/*
bool MQTTComm::attemptConnect(char * const broker_url, char * const client_id, int mqtt_port, bool quickstartMod, 
      int max_retries, int timeout)
{          
  int connack_rc;
  while ((connack_rc = connect(client_, ipstack_)) != MQTT_CONNECTION_ACCEPTED)
  {    
    if (connack_rc == MQTT_NOT_AUTHORIZED || connack_rc == MQTT_BAD_USERNAME_OR_PASSWORD) {
      // printf ("File: %s, Line: %d Error: %d\n\r",__FILE__,__LINE__, connack_rc);        
      return false; // don't reattempt to connect if credentials are wrong
    }

    if (max_retries--){
      wait(timeout);
    } else {  
      return false;
    } 
  }

  connected_ = true;
  return true;
}*/

bool MQTTComm::Disconnect() {
  if (!connected_)
    return true;
  return (client_.disconnect() == 0);
}

} // namespace tag12

#endif