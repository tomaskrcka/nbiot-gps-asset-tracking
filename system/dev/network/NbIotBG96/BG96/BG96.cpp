/* SPWFInterface Example
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "BG96.h"
#include "logging.h"
#include "mbed_debug.h"

// device id settings
#define ID_ON_FLASH 0

#if ID_ON_FLASH
const uint8_t DEVICE_ID[6] = {0x00, 0x80, 0xe1, 0xb7, 0xd3, 0x47};
#endif

// debug messages enable
#define DBG_MSG 0

#define BG96_CONNECT_TIMEOUT 150000 // based on datasheet - 150s
#define BG96_SEND_TIMEOUT 500
#define BG96_RECV_TIMEOUT \
  500  // some commands like AT&F/W takes some time to get the result back!
#define BG96_MISC_TIMEOUT 1000
#define BG96_SOCKQ_TIMEOUT 10000

#define MAX_ATTEMPTS 8

#define ACT_GSM 0
#define ACT_NBIOT 9

// extern  Serial pc;			//(SERIAL_TX, SERIAL_RX);

Timer main_timer;

DigitalOut BG96_reset(D7);

DigitalOut VBAT_3V8_EN(D11);

// DigitalOut BG96_W_DISABLE(D11);

DigitalOut BG96_PWRKEY(D10);

// extern DigitalOut myled;

BG96::BG96(PinName tx, PinName rx, PinName reset, PinName wakeup, bool debug)
    : _serial(tx, rx, 1024),
      _parser(_serial),
      _reset(reset, PIN_OUTPUT, PullNone, 1),
      _wakeup(wakeup, PIN_OUTPUT, PullNone, 0),
      dbg_on(debug) {
   // _serial.baud(115200);  // LICIO  FIXME increase the speed
   _serial.baud(115200);  // LICIO  FIXME increase the speed
  _parser.debugOn(debug);
}

static void delay_ms(uint32_t del) {
  do {
    wait_ms(1);
  } while (del--);
}

bool BG96::startup(int mode) {
  // Timer timer_s;
  // uint32_t time;
  _parser.setTimeout(BG96_MISC_TIMEOUT);
  /*Test module before reset*/
  if (!waitBG96Ready()) return false;

  wait(1.0);
  _parser.send("ATE0");
  int max_att = MAX_ATTEMPTS;
  while (max_att-- > 0) {
    if (_parser.recv("OK")) {
      // printf("ATE ok\r\n");
      LOG("ATE OK");
      break;
    }
  }

  if (max_att <= 0) return false;

  // pc.printf("waiting for network...\r\n");  
  // myled=0;
  // only to see cops data ... no matching or check
  _parser.send("AT+CFUN=0");
  max_att = MAX_ATTEMPTS;
  while (max_att--) {
    if (_parser.recv("OK")) {
      break;
    }
  }

  if (max_att <= 0) return false;

#if 0	
    _parser.send("AT+QIACT?");
    while (1) {
        if (_parser.recv("OK"))
            break;
    }
#endif
  /*Reset parser buffer*/
  _parser.flush();
  return true;
}

bool BG96::setConnection(uint32_t band, char * code, const char *apn, const char *username, const char *password, bool nbiot) {
  // Config
  int act = ACT_GSM;

  _parser.setTimeout(BG96_MISC_TIMEOUT);
  if (!(_parser.send("AT+CFUN=0") && _parser.recv("OK"))) {
    LOG("CFUN=0 set Error");
    // return false;
  }

  if (nbiot) {
    act = ACT_NBIOT;
    if (!(_parser.send("AT+QCFG=\"band\",0,0,%lu,1", band) && _parser.recv("OK"))) {
      LOG("band nok");
    }

    if (!(_parser.send("AT+QCFG=\"nwscanmode\",3,1") && _parser.recv("OK"))) {
      LOG("nwscanmode nok");
    }
  
    if (!(_parser.send("AT+QCFG=\"nwscanseq\",030102,1") && _parser.recv("OK"))) {
      LOG("nwscanseq nok");
    }
  
    if (!(_parser.send("AT+QCFG=\"iotopmode\",1,1") && _parser.recv("OK"))) {
      LOG("iotopmode NOK");
    }

    if (!(_parser.send("AT+QCFG=\"servicedomain\",1,1") && _parser.recv("OK"))) {
      LOG("servicedomain NOK");
    }
  } else {
    if (!(_parser.send("AT+QCFG=\"nwscanmode\",1,1") && _parser.recv("OK"))) {
      LOG("nwscanmode nok");
    }
  }

  if (!(_parser.send("AT+CFUN=1") && _parser.recv("OK"))) {
    LOG("CFUN set Error");
    return false;
  }

  if (!(_parser.send("AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",0", &apn[0],
                      &username[0], &password[0]) && _parser.recv("OK"))) {
    LOG("No possible to set APN");
    return false;
  }

  _parser.setTimeout(BG96_MISC_TIMEOUT*4);
  int max_attempt = 8;
  char oper[8];
  char msg[64];
  
  sprintf(oper, "\",%d", act); // ending of the COPS command
  while (max_attempt-- > 0) {
    memset(msg, 0, sizeof(msg));
    if (_parser.send("AT+COPS?") && _parser.read(msg, sizeof(msg))) {
      if (strstr(msg, oper) != NULL) {
        LOG("Already connected: %s", msg);
        return true;
      }
    }
    _parser.flush();
    LOG("next attempt: msg: %s", msg);
    wait(3);
  }

  _parser.setTimeout(BG96_MISC_TIMEOUT*30);
  if (!(_parser.send("AT+COPS=1,2,\"%s\",%d", code, act) && _parser.recv("OK"))) {
    LOG("ERROR COPS: AT+COPS=1,2,\"%s\" %d", code, act);
    return false;
  }

  _parser.setTimeout(BG96_MISC_TIMEOUT*5);
  if (_parser.send("AT+COPS?") && _parser.read(msg, sizeof(msg))) {
    if (strstr(msg, oper) != NULL) {
      LOG("Connected: %s", msg);
      return true;
    }
  }

  LOG("Not connected");
  _parser.setTimeout(BG96_MISC_TIMEOUT);
  return false;
}

bool BG96::hw_reset(void) {
 BG96_reset = 0;
  BG96_PWRKEY = 0;
  VBAT_3V8_EN = 0;

  delay_ms(200);

  LOG("HW reset BG96");
  // // pc.printf("HW reset BG96\r\n");

   BG96_reset = 1;

   VBAT_3V8_EN = 1;

  //    BG96_W_DISABLE = 0;

  BG96_PWRKEY = 1;

  delay_ms(500);

   BG96_reset = 0;

  delay_ms(5);

  return 1;
}

void BG96::wakeup(void) {
  /*BG96_PWRKEY = 0;
  delay_ms(200);
  BG96_PWRKEY = 1;*/
  reset();
}

bool BG96::reset(void) {
  // // pc.printf("\n\rInit BG96 ...\r\n");
  LOG("Reset BG96");
  hw_reset();
  return true;
}

bool BG96::waitBG96Ready(void) {
  // reset();
  // // pc.printf("Wait for ready...");
   LOG("waitBG96Ready");
  // after HW reset,if no RDY from BG96 program stops
  // green led lamps 0.5 seconds
  int max_attempts = MAX_ATTEMPTS;
  while (max_attempts-- > 0) {
    if (_parser.recv("RDY")) {
      // // pc.printf("BG96 ready\r\n");
      // myled = 0;
      LOG("BG96 ready");
      return true;
    }
    delay_ms(300);
    /*
                            if (myled == 0)
                                    myled = 1;
                            else
                                    myled = 0;
    */
  }
  return (max_attempts > 0 ? true : false);
}

char pdp_string[100];

bool BG96::connect() {
  int i = 0;
  char *search_pt;

  Timer timer_s;
  uint32_t time;

  _parser.setTimeout(BG96_MISC_TIMEOUT);
  // activate PDP context 1 ...
  // pc.printf("PDP activating ...\r\n");
  int pdp_retry = 0;
  int a = 1;
  while (a == 1) {
    _parser.send("AT+QIACT=1");
    timer_s.reset();
    timer_s.start();
    while (1) {
      if (_parser.recv("OK")) {
        a = 0;
        break;
      }

      time = timer_s.read_ms();
      uint32_t end_time = (BG96_MISC_TIMEOUT * (5 + (pdp_retry * 2)));
      if (time > end_time) {
        pdp_retry++;
        if (pdp_retry > 3) {
          // // pc.printf("ERROR --->>> PDP not valid, program stoppped!!\r\n");
          // // pc.printf("***********************************************");
          LOG("PDP not valid");
          return false;
        }
        break;
      }
    }
  }
  LOG("PDP started");
  return true;
}

bool BG96::disconnect(void) {
  // TODO add needed further action after ACT close command ...
  if (!(_parser.send("AT+QIDEACT=%d\r", 1) && _parser.recv("OK"))) {
    // debug_if(dbg_on, "BG96> error while disconnet...\r\n");
    return false;
  }
  // reset();
  return true;
}

bool BG96::setGPS(void) {
  LOG("active gps");
  int max_attempt = MAX_ATTEMPTS;

  while (!(_parser.send("AT+QGPS=2,90,500,0,1") && _parser.recv("OK"))) {
    // Activate GPS with default config
    if (max_attempt-- <= 0) {
      return false;
    }
    wait_ms(500);
  }

  LOG("activated gps");
  // gps activated;
  return true;
}

bool BG96::GPSOff() {
  LOG("gps off");

  if (!(_parser.send("AT+QGPSEND") && _parser.recv("OK"))) {
    return false;
  }

  return true;
}

bool BG96::getGPS(char *gps_pointer, int array_length) {
  bool res = true;
  if (!(_parser.send("AT+QGPSLOC=2") &&
        _parser.read(gps_pointer, array_length))) {
    // // pc.printf("No GPS Position: %s\r\n",gps);
    LOG("No GPS Position ERROR read %s", gps_pointer);
    res = false;
  }

  char * search_pt = strstr(gps_pointer, "OK\r\n");
  if (search_pt == NULL) {
    LOG("No GPS Position %s", gps_pointer);
    res = false;
  }
  
  // // pc.printf("GPS Position: %s\r\n",gps);
  // Should look like these: +QGPSLOC:
  // 101632.0,47.74811,9.58924,1.3,420.0,2,36.03,0.0,0.0,171218,09

  return res;
}

const char *BG96::getIPAddress(void) {
  // TODO this function needs further debug ... now always returns IP 0.0.0.0
  char act_string[32];
  // IPV4 mode
  uint8_t n1, n2, n3, n4;

  if (!(_parser.send("AT+QIACT?")
        // NOTE the %s on parser.read function returns only the first character
        // of string ...
        // TODO verify parser.read  function
        // here used only to check if commands is well processed by BG96
        && _parser.recv("+QIACT: %s", &act_string[0]) && _parser.recv("OK"))) {
    // debug_if(dbg_on, "BG96> getIPAddress error\r\n");
    return 0;
  }
  // TODO parse string to find IP ...
  // now this functions returns always IPV4 0.0.0.0
  n1 = n2 = n3 = n4 = 0;
  sprintf((char *)_ip_buffer, "%u.%u.%u.%u", n1, n2, n3, n4);

  return _ip_buffer;
}

#define IMEI_OFFS 2
#define IMEI_LEN 14 + IMEI_OFFS

const char *BG96::getMACAddress(void) {
  uint32_t n1, n2, n3, n4, n5, n6;

#if ID_ON_FLASH
  n1 = DEVICE_ID[0];  // 0x00;
  n2 = DEVICE_ID[1];  // 0x0b;
  n3 = DEVICE_ID[2];  // 0x57;
  n4 = DEVICE_ID[3];  // 0x55;
  n5 = DEVICE_ID[4];  // 0xdb;
  n6 = DEVICE_ID[5];  // 0x45;
#else
  char sn[32];
  memset(sn, 0, sizeof(sn));
  if (!(_parser.send("AT+CGSN") && _parser.read(&sn[0], IMEI_LEN) &&
        _parser.recv("OK"))) {
    // // pc.printf("BG96> IMEI error\r\n");
    return 0;
  }
/*
imei: 86642503006284
DEVICE ID is 00:0B:A5:89:04:C6
*/
  LOG("imei: %s", sn);
  n1 = 0x00;
  n2 = 0x0b;
  n3 =
      (uint8_t)(((sn[4 + IMEI_OFFS] & 0x0f) << 4 | (sn[5 + IMEI_OFFS] & 0x0f)) +
                ((sn[12 + IMEI_OFFS] & 0x0f) << 4));
  n4 = (uint8_t)(
      ((sn[6 + IMEI_OFFS] & 0x0f) << 4 | (sn[7 + IMEI_OFFS] & 0x0f)) +
      (uint8_t)((sn[0 + IMEI_OFFS] & 0x0f) << 4 | (sn[1 + IMEI_OFFS] & 0x0f)));
  n5 =
      (uint8_t)(((sn[8 + IMEI_OFFS] & 0x0f) << 4 | (sn[9 + IMEI_OFFS] & 0x0f)) +
                ((sn[13 + IMEI_OFFS] & 0x0f)));
  n6 = (uint8_t)(
      ((sn[10 + IMEI_OFFS] & 0x0f) << 4 | (sn[11 + IMEI_OFFS] & 0x0f)) +
      (uint8_t)((sn[2 + IMEI_OFFS] & 0x0f) << 4 | (sn[3 + IMEI_OFFS] & 0x0f)));
#endif
  sprintf((char *)_mac_buffer, "%02lX:%02lX:%02lX:%02lX:%02lX:%02lX", n1, n2, n3, n4, n5,
          n6);
  LOG("DEVICE ID is %s",_mac_buffer);
  return _mac_buffer;
}

bool BG96::isConnected(void) { return getIPAddress() != 0; }
uint8_t sock_id = 0;
bool socket_opening = false;

bool BG96::open(const char *type, int *id, const char *addr, int port) {
  Timer timer;
  timer.start();
  socket_closed = 0;
  uint32_t time;
  bool res;
  char send_type[16];
  const char *send_type_pt = send_type;
  int result;

  // check socket type ...
  switch (*type) {
    case 'u':
      send_type_pt = "UDP";
      // UDP on sock id = 1
      sock_id = 1;
      break;
    case 't':
      send_type_pt = "TCP";
      // TCP on sock id = 0
      sock_id = 0;
      break;
    default:
      send_type_pt = "TCP";
      sock_id = 0;
      break;
  };
  _parser.setTimeout(BG96_SOCKQ_TIMEOUT*2);

  // open socket for context 1
  while (1) {
    if ((_parser.send("AT+QIOPEN=1,%d,\"%s\",\"%s\",%d,0,0\r", sock_id,
                      send_type_pt, addr, port)) &&
        (_parser.recv("OK")))

    {
      while (1) {
        res = _parser.recv("+QIOPEN: %d,%d", id, &result);
        if (res == true) {
          // // pc.printf("Open socket type %s #%d result = %d\r\n",
          // send_type_pt, *id, result);
          LOG("Open socket type %s #%d result = %d",send_type_pt, *id, result);
          if (result == 0) {
            // wait for socket open result ... start RECV timeout timer
            socket_opening = true;
            main_timer.reset();
            main_timer.start();
            return true;
          } else {
            int s_id = *id;
            // if (result == 563)
            close(s_id);
            return false;
          }
        }

        time = timer.read_ms();
        if (time > BG96_CONNECT_TIMEOUT) {
          LOG("Timeout sock open!!");
          return false;
        }
      }
    }
    LOG("error OPEN");
    return false;
  }
}
uint8_t err_counter = 0;

bool BG96::socket_openstream(int id, unsigned size) {
  char _buf[64];

  // char result_string[64];
  char *search_pt;
  _parser.setTimeout(BG96_SEND_TIMEOUT);

  _parser.send("AT+QISEND=%d,0", id);
  memset(_buf, 0, sizeof(_buf));
  for (unsigned i = 0; i < 3; i++) {
    _parser.read(_buf, sizeof(_buf));
    search_pt = strstr(_buf, "OK\r\n");
    if (search_pt != 0) {
      break;
    }
  }

  if (search_pt == 0) {
    LOG("Stream was not opened !!");
    return false;
  }

  search_pt = strstr(_buf, ",0");
  if (search_pt == 0) {
    LOG("Stream was not opened !!");
    return false;
  }

  memset(_buf, 0, sizeof(_buf));
  // here send data to socket ...
  int buf_size = sprintf((char *)_buf, "AT+QISEND=%d,%d\r", id, size);

  // May take more try if device is busy
  for (unsigned i = 0; i < 3; i++) {
    if (_parser.write((char *)_buf, buf_size) >= 0) {
      if (_parser.recv("> ")) {
          LOG("Stream has been opened");
          return true;
        }
      }
    // if send fails, wait before further attempt
    // // pc.printf("Send retry #%d\r\n", (i+1));
    wait_ms(2000);
  }

  return false;
}

int BG96::socket_writestream(int id, const char * data, uint32_t amount) {
  return _parser.write((char *)data, (int)amount);
}

int BG96::socket_openreadstream(int id) {
  int id_tmp;

  if (!(_parser.recv("+QIURC: \"recv\",%u", &id_tmp))) 
    return -1;

  if (id_tmp > 1)
    return -1;

  int recv_amount;

  if (!(_parser.send("AT+QIRD=%d\r", id)  // send a query (will be required for secure sockets)
      && _parser.recv("+QIRD: %u", &recv_amount))) {
    return -1;
  }
  
  return recv_amount;
}

int BG96::socket_readstream(int id, const char * data, uint32_t amount) {
  return _parser.read((char *)data, amount);
}

bool BG96::socket_closereadstream(int id) {
  if (_parser.recv("OK")) {
    LOG("REcv!");
    return true;
  }

  return false;
}

bool BG96::socket_closestream(int id) {
  if (_parser.recv("SEND OK")) {
    LOG("Sent!");
    return true;
  }

  close(id);
  socket_closed = 1;
  return false;
}


bool BG96::send(int id, const void *data, uint32_t amount) {
  char _buf[64];

  // char result_string[64];
  char *search_pt;
  _parser.setTimeout(BG96_SEND_TIMEOUT);

  // pclog.printf("Sending %d len ...", amount);

  // check if previous sent is good, otherwise after 5 consecutive fails close
  // socket and error!!
  if (err_counter > 5) {
    // // pc.printf("Closing socket #%d\r\n", id);
    close(id);
    socket_closed = 1;
    err_counter = 0;
    return false;
  }

#if DBG_MSG

  uint8_t *dt = (uint8_t *)data;
  // these tests and print is only for debug purposes
  if (amount == 2) {
    if ((dt[0] == 0xc0) && (dt[1] == 0x00))
    // // pc.printf("\r\n\nPING ");
  }

  _parser.send("AT+COPS?");
  while (1) {
    if (_parser.recv("OK")) {
      break;
    }
  }

#endif

  _parser.send("AT+QISEND=%d,0", id);
#if 0
				err_counter = 0;
				while(1)
				{
					if( _parser.recv("+QISEND: %s", &result_string[0])
            && _parser.recv("OK")) 
						{
							break;
						}	
				}
#else

  int i = 0;
  memset(_buf, 0, sizeof(_buf));
  const char OK_str[] = {"OK\r\n"};
  const char OK0_str[] = {",0"};
  while (1) {
    // read and store answer
    //_parser.read(&result_string[i], 1);
    //i++;

    _parser.read(_buf, sizeof(_buf));
    // if OK rx, end string; if not, program stops
    search_pt = strstr(_buf, OK_str);
    if (search_pt != 0) {
      break;
    }
    // TODO: add timeout if no aswer from module!!
  }

  // if send fails, the string doesn't have ",0" sequence, then
  search_pt = strstr(_buf, OK0_str);
  if (search_pt == 0) {
    err_counter++;
  } else {
    err_counter = 0;
  }

#endif
  memset(_buf, 0, sizeof(_buf));
  // here send data to socket ...
  int buf_size = sprintf((char *)_buf, "AT+QISEND=%d,%d\r", id, amount);

  // May take more try if device is busy
  for (unsigned i = 0; i < 3; i++) {
    if (_parser.write((char *)_buf, buf_size) >= 0) {
      if (_parser.recv("> ")) {
        if ((_parser.write((char *)data, (int)amount) >= 0) &&
            (_parser.recv("SEND OK"))) {
#if DBG_MSG
// print for debug
// // pc.printf("Sent!\r\n");
#endif
          return true;
        }
      }
    }
    // if send fails, wait before further attempt
    // // pc.printf("Send retry #%d\r\n", (i+1));
    wait_ms(2000);
  }

// FAIL to send data to socket, after 3 retry!!
#if DBG_MSG
  // these test and print only for debug
  _parser.send("AT+COPS?");
  while (1) {
    if (_parser.recv("OK")) {
      break;
    }
  }
  _parser.send("AT+QISTATE?");
  while (1) {
    if (_parser.recv("OK")) {
      break;
    }
  }
#endif

  // // pc.printf("Closing socket #%d\r\n", id);
  close(id);
  socket_closed = 1;
  return false;
}

uint8_t local_data_store[16];
uint8_t local_amount = 0;
uint8_t local_pt = 0;

int32_t BG96::recv(int id, void *data, uint32_t amount) {
  uint32_t recv_amount = 0;
  uint32_t now_time;
#if DBG_MSG
  uint8_t *dt = (uint8_t *)data;
#endif
  // received data pending??
  if (local_amount > 0) {
    memcpy(data, &local_data_store[local_pt], amount);
    local_amount = local_amount - amount;
    local_pt = local_pt + amount;
    // return pending data...
    return amount;
  }

  local_amount = 0;
  local_pt = 0;

  if (socket_closed) {
    socket_closed = 0;
    return -3;
  }
  // wait dato from open socket request ... check timeout
  if (socket_opening == true) {
    now_time = main_timer.read_ms();
    if (now_time > BG96_CONNECT_TIMEOUT) {
      socket_opening = false;
      // // pc.printf("-->>> SOCK tout\r\n");
      main_timer.stop();
      return -3;
    }
  }

  /*
  int par_timeout = _parser.getTimeout();
  _parser.setTimeout(0);
  _parser.setTimeout(par_timeout);
  */
  switch (id) {
    case 0:
      if (!(_parser.recv("+QIURC: \"recv\",0"))) return -1;
      break;
    case 1:
      if (!(_parser.recv("+QIURC: \"recv\",1"))) return -1;
      break;
    default:
      return -1;
      // break;
  }

  if ((_parser.send("AT+QIRD=%d,%d\r",
                    id, amount))  // send a query (will be required for secure sockets)
      && (_parser.recv("+QIRD: %u", &recv_amount))) {
    _parser.read((char *)data, recv_amount);
    while (_parser.recv("OK"))
      ;

#if DBG_MSG
    if ((recv_amount == 2) && (dt[0] == 0xd0) && (dt[1] == 0x00))
    // pc.printf("PING Recv!\r\n");
#endif

      // if data amount received more than expected, store it for next call of
      // recv function
      if (recv_amount > amount) {
        local_amount = recv_amount - amount;
        local_pt = local_pt + amount;
        memcpy(local_data_store, data, recv_amount);
        recv_amount = amount;
      }
    socket_opening = false;
    return recv_amount;
  }
  return -2;
}

bool BG96::close(int id) {
  uint32_t recv_amount = 0;
  void *data = NULL;

  _parser.setTimeout(BG96_SOCKQ_TIMEOUT * 2);
  _parser.flush();
  /* socket flush */
  if (!(_parser.send("AT+QIRD=%d,1000\r",
                     id)  // send a query (will be required for secure sockets)
        && _parser.recv("+QIRD: %u", &recv_amount))
      //&& _parser.recv("OK"))
  ) {
    return -2;
  }

  if (recv_amount > 0) {
    data = malloc(recv_amount + 4);
    //       printf ("--->>>Close flushing recv_amount: %d  \n\r",recv_amount);
    if (!((_parser.read((char *)data, recv_amount) > 0) &&
          _parser.recv("OK"))) {
      free(data);
      return -2;
    }
    free(data);
  } else {
    if (!(_parser.recv("OK"))) return -2;
  }

  // May take a second try if device is busy or error is returned
  for (unsigned i = 0; i < 2; i++) {
    if ((_parser.send("AT+QICLOSE=%d", id)) && _parser.recv("OK")) {
      socket_closed = 1;
      return true;
    } else {
      if (_parser.recv("ERROR")) {
        debug_if(dbg_on, "BG96> ERROR!!!!\r\n");
        return false;
      }
    }
    // TODO: Deal with "ERROR: Pending data" (Closing a socket with pending
    // data)
  }
  return false;
}

bool BG96::readable() { return _serial.readable(); }

bool BG96::writeable() { return _serial.writeable(); }

void BG96::shutdown() {
  _parser.setTimeout(BG96_RECV_TIMEOUT * 10);
  _parser.send("AT+QPOWD");
  _parser.recv("POWERED DOWN");
  wait(1);
  BG96_PWRKEY = 0;
  VBAT_3V8_EN = 0;

  delay_ms(200);
}

/*
bool BG96::sendHttpPost(char *url, int urlLength, char * data, int size) {
  if (!(_parser.send("AT+QHTTPCFG=\"contextid\",1") || !_parser.recv("OK"))) {
    LOG("contextid false");
    return false;
  }
  
  if (!(_parser.send("AT+QHTTPPOST=?") && _parser.recv("OK"))) {
    LOG("QHTTPPOST=? false");
    return false;
  }
  
  if (!(_parser.send("AT+QHTTPCFG=\"requestheader\",1") && _parser.recv("OK"))) {
    LOG("QHTTPCFG false");
    return false;
  }

  int max_att = 10;
  while (1) {
    if (!_parser.send("AT+QHTTPURL=%d,80", urlLength)) {
      LOG("QHTTPURL false");
      return false;
    }

    while (!_parser.recv("CONNECT") && (max_att-- >= 0)) {
      wait_ms(500);
    }

    if (max_att <= 0) {
      LOG("QHTTPURL CONNECT false");
      max_att = 10;
    } else {
      LOG("QHTTPURL CONNECT OK");
      break;
    }
  }

  // May take more try if device is busy
  for (unsigned i = 0; i < 3; i++) {
    if (_parser.write(url, urlLength) >= 0) {
      break;
    }
    wait_ms(2000);
  }

  if (!_parser.recv("OK")) {
    LOG("OK false");
    return false;
  }

  char header[] =
  "POST /t/wi095-1550826983/post HTTP/1.1\r\nHost: ptsv2.com\r\nAccept: */
  //*\r\nUser-Agent: tag12\r\nConnection: Keep-Alive\r\nContent-Type: application/json\r\n\r\n";
  /*
  char tmp[255];
  memset(tmp, 0, sizeof(tmp));
  strcpy(tmp, header);
  strcat(tmp, data);
  int size_tmp = strlen(tmp);

  max_att = 10;
  while (1) {
    if (!_parser.send("AT+QHTTPPOST=%d,80,80", size_tmp)) {
      LOG("QHTTPPOST false");
      return false;
    }

    while (!_parser.recv("CONNECT") && (max_att-- >= 0)) {
      wait_ms(500);
    }

    if (max_att <= 0) {
      LOG("QHTTPPOST CONNECT false");
      max_att = 10;
    } else {
      LOG("QHTTPPOST CONNECT OK");
      break;
    }
  }

  // May take more try if device is busy
  for (unsigned i = 0; i < 3; i++) {
    if (_parser.write(tmp, size_tmp) >= 0) {
      break;
    }
    wait_ms(2000);
  }

  max_att = MAX_ATTEMPTS;
  while (!_parser.recv("+QHTTPPOST") && max_att-- > 0) {
    // LOG("QHTTPPOST write false");
    wait_ms(100);
  }

  if (max_att <= 0) {
     LOG("QHTTPPOST write false");
     return false;
  }

  if (!(_parser.send("AT+QHTTPREAD=80", urlLength))) {
    LOG("QHTTPREAD read false");
    return false;
  }

  char data_tmp[128];
  memset(data_tmp, 0, sizeof(data_tmp));
  char * search_pt;
  max_att = MAX_ATTEMPTS;
  while(max_att-- > 0) {
    _parser.read(data_tmp, sizeof(data_tmp));
    LOG("%s", data_tmp);
    search_pt = strstr(data_tmp, "OK\r\n");
    if (search_pt != 0) {
      break;
    }
  }

  return true;
}*/

bool BG96::go2PSM() {
  if (!(_parser.send("AT+CPSMS=1,,,\"00000100\",\"00001111\"") && _parser.recv("OK"))) {
    return false;
  }

  return true;
}

bool BG96::creg(char * response, int length) {
  
  if (!(_parser.send("AT+CEREG=2") && _parser.recv("OK"))) {
    LOG("creg=2 failed");
    // return false;
  }

  if (!(_parser.send("AT+CEREG?") &&
        _parser.read(response, length))) {
    LOG("creg: res: %s", response);
    return false;
  }
  LOG("creg: res: %s", response);
  return true;
}

int BG96::getVoltage() {
  char buf[64];
  int voltage = -1;

  if (!(_parser.send("AT+CBC") &&
        _parser.read(buf, sizeof(buf)) && _parser.recv("ERROR"))) {
    // // pc.printf("No GPS Position: %s\r\n",gps);
    return voltage;
  }

  sprintf(buf, "+CBC: 0,74,%d", &voltage);

  return voltage;
}