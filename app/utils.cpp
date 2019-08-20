#include "utils.h"

#include <ctime>

namespace tag12 {

uint32_t  Utils::ConvertGPSTime(char * gpstime, char *gpsdate) {
  int hh, mm, ss;
  int dd, MM, yy;

  sscanf(gpstime, "%02d%02d%02d", &hh, &mm, &ss);
  sscanf(gpsdate, "%02d%02d%02d", &dd, &MM, &yy);

  struct tm t = {0};  // Initalize to all 0's
  t.tm_year = (2000 + yy) - 1900;  // This is year-1900, so 112 = 2012
  t.tm_mon = MM - 1;
  t.tm_mday = dd;
  t.tm_hour = hh;
  t.tm_min = mm;
  t.tm_sec = ss;
  return (uint32_t) mktime(&t);
}

} // namespace tag12