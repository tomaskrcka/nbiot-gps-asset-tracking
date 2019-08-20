

// standby
// gps
// acc
// GSM
// mqtt
// configuration
// initialization
// network stack
#include "mbed.h"

#include "logging.h"
#include "app/app.h"

// Serial pc(SERIAL_TX, SERIAL_RX);

int main() {
  LOG_INIT;
  /*BG96Interface bg96if_(D8, D2, false);
  tag12::MQTTComm mqtt_comm_(&bg96if_);*/
  LOG("*****************************************");
#ifdef VODAFONE
  LOG("TAG12 prototype Version: " VERSION " Vodafone" );
#else
  LOG("TAG12 prototype Version: " VERSION " T-MOBILE" );
#endif

  tag12::App app;
  app.Run();

  // app.RunTest();

  LOG("Point of neverending");

  // never reached part of the app
  while (true) {
    wait(1);
  }

  return 0;
}