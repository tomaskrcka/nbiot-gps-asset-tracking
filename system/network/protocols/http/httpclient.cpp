#include "httpclient.h"

#include "logging.h"

#include "config_default.h"

#define POST_HEADER "POST " HTTP_REST_API_URL " HTTP/1.1\r\nHost: " HTTP_REST_API_HOSTADDR "\r\n" \
        "Content-Type: application/json\r\nConnection:close\r\nContent-Length: "

#define POST_EOH "%d\r\n\r\n"

#define GET_HEADER "GET %s HTTP/1.1\r\nHost: " HTTP_REST_API_HOSTADDR "\r\nConnection:close\r\n"

#define HTTP_EOT "\r\n\x1A"

HttpClient::HttpClient(BG96Interface* const ipstack) : ipstack_(ipstack) {
  // Nothing
}

bool HttpClient::HttpPost(const char * data, int size)
{
  char buf[16];
  int size_packet = sprintf(buf, "%d\r\n\r\n" , size);    
  
  void *socket;
  if (ipstack_->socket_open(&socket, NSAPI_TCP) < 0) {
    LOG("ERROR HttpPost socket_open");
    return false;
  }
  int err;
  int max_attempt = 2;
  while ((err = ipstack_->socket_connect(socket, HTTP_REST_API_HOSTADDR, HTTP_PORT)) != 0) {
    LOG("error connection: %d", err);
    if (max_attempt-- <= 0) 
      return false;
  }

  LOG("connected http");
  if (!ipstack_->socket_openstream(socket, sizeof(POST_HEADER) + size_packet + size + sizeof(HTTP_EOT) - 1)) {
    LOG("ERROR HttpPost socket_openstream");
    return false;
  }

  ipstack_->socket_writestream(socket, (char*)POST_HEADER, sizeof(POST_HEADER) - 1);
  ipstack_->socket_writestream(socket, (char*)buf, size_packet);
  ipstack_->socket_writestream(socket, (char*)data, size);
  ipstack_->socket_writestream(socket, (char*)HTTP_EOT, sizeof(HTTP_EOT));
  ipstack_->socket_closestream(socket);

  int sent_size = sizeof(POST_HEADER) + size_packet + size + sizeof(HTTP_EOT) - 1;
  LOG("sent http size: %d", (sent_size));

  wait_ms(2000);
#ifdef false
  memset(buf, 0, sizeof(buf));
  ipstack_->socket_openreadstream(socket);
  while(ipstack_->socket_readstream(buf, sizeof(buf)-1)) {
    LOG("buf: %s", buf);
    memset(buf, 0, sizeof(buf));
  }
#endif

  ipstack_->socket_close(socket);
  return true;
}

bool HttpClient::HttpGet(const char * link, char * data, int size)
{
  char buf[128];  
  void *socket;
  if (ipstack_->socket_open(&socket, NSAPI_TCP) < 0) {
    LOG("ERROR socket_open");
    return false;
  }

  int size_header = snprintf(buf, sizeof(buf), GET_HEADER, link);
  LOG("%s", buf);

  int err;
  int max_attempt = 2;
  while ((err = ipstack_->socket_connect(socket, HTTP_REST_API_HOSTADDR, HTTP_PORT)) != 0) {
    LOG("error connection: %d", err);
    if (max_attempt-- <= 0) 
      return false;
  }
  LOG("GET connected http");
  
  if (!ipstack_->socket_openstream(socket, size_header + sizeof(HTTP_EOT) - 1)) {
    LOG("ERROR openstream");
    return false;
  }

  ipstack_->socket_writestream(socket, (char*)buf, size_header);
  ipstack_->socket_writestream(socket, (char*)HTTP_EOT, sizeof(HTTP_EOT));
  ipstack_->socket_closestream(socket);
  wait_ms(2000);
  memset(data, 0, size);
  
  int recv_amount = ipstack_->socket_openreadstream(socket);
  if (recv_amount < 0) {
    LOG("ERROR openreadstream");
    return false;
  }

  bool cpy = false;
  char * src = NULL;
  char * dest = NULL;
  char * pos = data;
  int read_num;
  int write_size = size;
  LOG("get: %s", buf);
  memset(buf, 0, sizeof(buf));
  while ((read_num = ipstack_->socket_readstream(buf, sizeof(buf)-1))) {
    if ((src = strstr(buf, "{")) != NULL) {
      cpy = true;
    } else {
      src = buf;
    }

    if ((dest = strstr(buf, "}")) != NULL) {
      write_size = (dest - src) + 1;
    } 

    if (cpy) {
      strncpy(pos, src, write_size);
      pos += (&buf[read_num-1] - src) + 1;
      write_size -= (pos - data);
    }

    if (dest != NULL)
      break;
    memset(buf, 0, sizeof(buf));
  }
  
  // ipstack_->socket_recv(socket, data, size);
  ipstack_->socket_close(socket);
  return true;
}
