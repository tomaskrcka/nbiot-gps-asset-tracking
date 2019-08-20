#include "httpcomm.h"
#include "../config/config_default.h"
#include "logging.h"
#include "../../system/network/protocols/http/httpclient.h"

#define HTTP_MAX_PAYLOAD_SIZE 128

namespace tag12 {

HTTPComm::HTTPComm(BG96Interface* const ipstack) : ipstack_(ipstack), connected_(false) {
  // Nothing
}

bool HTTPComm::Publish(DataSend data)
{
  char tmp_buf[HTTP_MAX_PAYLOAD_SIZE];
  memset(tmp_buf, 0, sizeof(tmp_buf));
  int tmp_size = 0;

  if (data.gps_data.valid) {
    tmp_size = snprintf(tmp_buf, sizeof(tmp_buf),
      "{\"id\":\"%s\",\"ts\":%ld,\"lat\":%lf,\"lon\":%lf,\"bat\":%.3lf, \"net\": %d,\"cell\":\"%X\",\"temp\":%.3lf", data.mac_address, data.timestamp, 
      data.gps_data.lat, data.gps_data.lon, data.voltage, data.nbiot, data.cell_id, data.temperature);
  } else {
    tmp_size = snprintf(tmp_buf, sizeof(tmp_buf),
      "{\"id\":\"%s\",\"bat\":%.3lf,\"net\": %d,\"cell\":\"%X\",\"temp\":%.3lf", data.mac_address, data.voltage, data.nbiot, 
      data.cell_id, data.temperature);
  }
  
  if (data.log_err.intValue > 0) {
    tmp_size += snprintf(tmp_buf+tmp_size, sizeof(tmp_buf) - tmp_size,
      ",\"errFlags\":%d}", data.log_err.intValue);
  } else {
    tmp_size += snprintf(tmp_buf+tmp_size, sizeof(tmp_buf) - tmp_size, "}");
  }

  LOG("send: %s", tmp_buf);
  HttpClient client(ipstack_);

  return client.HttpPost(tmp_buf, tmp_size);
}

bool HTTPComm::GetConfig(const char * mac, char * data, int size) {
  HttpClient client(ipstack_);
  char buf[82];
  snprintf(buf, sizeof(buf), HTTP_REST_API_URL_CONFIG "%s", mac);
  return client.HttpGet(buf, data, size);
}

int HTTPComm::Connect() { 
  // no connection to HTTP
  connected_ = true;
  return 0;
}

bool HTTPComm::Disconnect() {
  connected_ = false;
  return true;
}

} // namespace tag12