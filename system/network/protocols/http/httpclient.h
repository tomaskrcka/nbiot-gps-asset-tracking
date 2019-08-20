#ifndef PROTOCOL_HTTP_CLIENT_H
#define PROTOCOL_HTTP_CLIENT_H

#include "../system/dev/network/NbIotBG96/BG96Interface.h"

class HttpClient {
 public:
  HttpClient(BG96Interface* const ipstack);
  bool HttpPost(const char * data, int size);
  bool HttpGet(const char * link, char * data, int size);

 private:
  BG96Interface* const ipstack_;
};

#endif