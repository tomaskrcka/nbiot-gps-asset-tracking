#ifndef APP_UTILS_H
#define APP_UTILS

#include <stdio.h>

namespace tag12 {

class Utils {
public:
  static uint32_t ConvertGPSTime(char * gpstime, char * gpsdate);
};

} // namespace tag12

#endif