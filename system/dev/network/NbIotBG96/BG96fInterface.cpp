/* mbed Microcontroller Library
 * Copyright (c) 20015 ARM Limited
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

/**
 ******************************************************************************
 * @file    BG96Interface.cpp
 * @author  STMicroelectronics
 * @brief   Implementation of the NetworkStack for the BG96 Device
 ******************************************************************************
 * @copy
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
 * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2016 STMicroelectronics</center></h2>
 ******************************************************************************
 */

#include "BG96Interface.h"

#include "logging.h"
// Various timeouts for different BG96 operations
// NO NEEDED HERE!!
/*
#define BG96_CONNECT_TIMEOUT 20000
#define BG96_SEND_TIMEOUT    500
#define BG96_RECV_TIMEOUT    500
#define BG96_MISC_TIMEOUT    15000
*/

/** BG96_socket class
 *  Implementation of BG96 socket structure
 */
struct BG96_socket {
  int id;
  int server_port;
  nsapi_protocol_t proto;
  bool connected;
};

/**
 * @brief  BG96SAInterface constructor
 * @param  tx: Pin USART TX
 *         rx: Pin USART RX
 *         rst: reset pin for BG96 module
 *         wkup: reset pin for BG96 module
 *         rts: Pin USART RTS
 *         debug : not used
 * @retval none
 */
BG96Interface::BG96Interface(PinName tx, PinName rx, bool debug)
    : _BG96(tx, rx, PA_7, PB_6, debug) {
  memset(_ids, 0, sizeof(_ids));
  isInitialized = false;
  isListening = false;
}

BG96Interface::BG96Interface(PinName tx, PinName rx, PinName reset,
                             PinName wakeup, bool debug)
    : _BG96(tx, rx, reset, wakeup, debug) {
  memset(_ids, 0, sizeof(_ids));
  isInitialized = false;
  isListening = false;
}

/**
 * @brief  BG96SAInterface destructor
 * @param  none
 * @retval none
 */
BG96Interface::~BG96Interface() {}

/**
* @brief  init function
          initializes BG96 FW and module
* @param  none
* @retval error value
*/
int BG96Interface::init(void) {
  int max_attemps = 5;
  LOG("Wait for ready...");
  // _BG96.wakeup();
  while (max_attemps-- > 0) {
    if (_BG96.startup(0)) {
      isInitialized = true;
      return true;
    }

    if (max_attemps == 3) {
      _BG96.reset();
    }
  }

  return ((max_attemps <= 0) ? 0 : 1);
  // else return NSAPI_ERROR_DEVICE_ERROR;
}

void BG96Interface::wakeup() {
  _BG96.wakeup();
}

void BG96Interface::shutdown() {
  _BG96.shutdown();
}

/**
* @brief  network connect
          connects to Access Point

* @retval NSAPI Error Type
*/
int BG96Interface::connect() {
  // initialize the device before connecting
  return (_BG96.connect());
}

bool BG96Interface::setConnection(uint32_t band, char * code, const char *apn, const char *username, const char *password, bool nbiot) {
  if (!isInitialized) {
    if (!init()) 
      return false;
  }

  return _BG96.setConnection(band, code, apn, username, password, nbiot);
}

/**
* @brief  network disconnect
          disconnects from Access Point
* @param  none
* @retval NSAPI Error Type
*/
int BG96Interface::disconnect() { return (_BG96.disconnect()); }

/**
 * @brief  Get the local IP address
 * @param  none
 * @retval Null-terminated representation of the local IP address
 *         or null if not yet connected
 */
const char *BG96Interface::get_ip_address() { return _BG96.getIPAddress(); }
/**
 * @brief  Set GPS active
 * @param  none
 * @retval NSAPI Error Type
 */
bool BG96Interface::setGPS() { return _BG96.setGPS(); }
/**
 * @brief  Set GPS active
 * @param  none
 * @retval NSAPI Error Type
 */
int BG96Interface::getGPS(char *gps_pointer, int array_lenght) {
  return (_BG96.getGPS((char *)gps_pointer, (int)array_lenght));
}

/**
 * @brief  Get battery voltage
 * @param  none
 * @retval voltage in mV or -1 if error
 */
int BG96Interface::getVoltage() {
  return _BG96.getVoltage();
}

bool BG96Interface::GPSOff() {
  return _BG96.GPSOff();
}

/**
 * @brief  Get the MAC address
 * @param  none
 * @retval Null-terminated representation of the MAC address
 *         or null if not yet connected
 */
const char *BG96Interface::get_mac_address() { return _BG96.getMACAddress(); }

/**
 * @brief  open a socket handle
 * @param  handle: Pointer to handle
 *         proto: TCP/UDP protocol
 * @retval NSAPI Error Type
 */
int BG96Interface::socket_open(void **handle, nsapi_protocol_t proto) {
  int id = -1;

  struct BG96_socket *socket = new struct BG96_socket;
  if (!socket) {
    return NSAPI_ERROR_NO_SOCKET;
  }

  socket->id = id;
  socket->server_port = id;
  socket->proto = proto;
  socket->connected = false;
  *handle = socket;
  return 0;
}

/**
 * @brief  connect to a remote socket
 * @param  handle: Pointer to socket handle
 *         addr: Address to connect to
 * @retval NSAPI Error Type
 */
/*
int BG96Interface::socket_connect(void *handle, const SocketAddress &addr) {
  return socket_connect(handle, addr.get_ip_address(), addr.get_port());
}*/

int BG96Interface::socket_connect(void *handle, const char * addr, int port) {
  int sock_id = 99;
  struct BG96_socket *socket = (struct BG96_socket *)handle;

  const char *proto =
      (socket->proto == NSAPI_UDP) ? "u" : "t";  //"s" for secure socket?

  if (!_BG96.open(proto, &sock_id, addr, port)) {
    ;  // sock ID is allocated NOW
    return NSAPI_ERROR_DEVICE_ERROR;
  }

  if (sock_id <= BG96SA_SOCKET_COUNT) {
    socket->id = sock_id;  // the socket ID of this Socket instance
    _ids[socket->id] = true;
    socket->connected = true;
  } else
    return NSAPI_ERROR_NO_SOCKET;

  return 0;
}



/**
 * @brief  bind to a port number and address
 * @param  handle: Pointer to socket handle
 *         proto: address to bind to
 * @retval NSAPI Error Type
 */
/*
int BG96Interface::socket_bind(void *handle, const SocketAddress &address) {
  struct BG96_socket *socket = (struct BG96_socket *)handle;
  socket->server_port = address.get_port();
  return 0;
}
*/

/**
 * @brief  start listening on a port and address
 * @param  handle: Pointer to handle
 *         backlog: not used (always value is 1)
 * @retval NSAPI Error Type
 */
int BG96Interface::socket_listen(void *handle, int backlog) {
  return NSAPI_ERROR_UNSUPPORTED;
}

/**
 * @brief  accept connections from remote sockets
 * @param  handle: Pointer to handle of client socket (connecting)
 *         proto: handle of server socket which will accept connections
 * @retval NSAPI Error Type
 */
/*nsapi_error_t BG96Interface::socket_accept(nsapi_socket_t server,
                                        nsapi_socket_t *handle, SocketAddress
   *address)*/
int BG96Interface::socket_accept(void **handle, void *server) {
  return NSAPI_ERROR_UNSUPPORTED;
}

/**
 * @brief  close a socket
 * @param  handle: Pointer to handle
 * @retval NSAPI Error Type
 */
int BG96Interface::socket_close(void *handle) {
  struct BG96_socket *socket = (struct BG96_socket *)handle;
  int err = 0;

  if (socket->id != -1) {
    if (_BG96.close(socket->id)) {
      if (socket->id == SERVER_SOCKET_NO)
        isListening = false;
      else
        _ids[socket->id] = false;
    } else
      err = NSAPI_ERROR_DEVICE_ERROR;
  }

  delete socket;
  return err;
}

/**
 * @brief  write to a socket
 * @param  handle: Pointer to handle
 *         data: pointer to data
 *         size: size of data
 * @retval no of bytes sent
 */
int BG96Interface::socket_send(void *handle, const void *data, unsigned size) {
  struct BG96_socket *socket = (struct BG96_socket *)handle;
  // int err;

  /*if(socket->id==SERVER_SOCKET_NO)
      {
          if(socket->server_port==-1 || !isListening)
              return NSAPI_ERROR_NO_SOCKET; //server socket not bound or not
  listening

          err = _BG96.socket_server_write((uint16_t)size, (char*)data);
      }
  else
      {
          err = _BG96.send(socket->id, (char*)data, (uint32_t)size);
      }*/
  if (!_BG96.send(socket->id, (char *)data, (uint32_t)size)) {
    return NSAPI_ERROR_DEVICE_ERROR;
  }

  return size;
}

bool BG96Interface::socket_openstream(void *handle, unsigned size) {
  struct BG96_socket *socket = (struct BG96_socket *)handle;
  return _BG96.socket_openstream(socket->id, size);
}

int BG96Interface::socket_writestream(void *handle, const char * data, uint32_t amount) {
  struct BG96_socket *socket = (struct BG96_socket *)handle;
  return _BG96.socket_writestream(socket->id, data, amount);
}

bool BG96Interface::socket_closestream(void *handle) {
  struct BG96_socket *socket = (struct BG96_socket *)handle;
  return _BG96.socket_closestream(socket->id);
}

int BG96Interface::socket_openreadstream(void *handle) {
  struct BG96_socket *socket = (struct BG96_socket *)handle;
  return _BG96.socket_openreadstream(socket->id);
}

int BG96Interface::socket_readstream(const char * data, uint32_t amount) {
  return _BG96.socket_readstream(0, data, amount);
}

bool BG96Interface::socket_closereadstream() {
  return _BG96.socket_closereadstream(0);
}

/**
 * @brief  receive data on a socket
 * @param  handle: Pointer to handle
 *         data: pointer to data
 *         size: size of data
 * @retval no of bytes read
 */
int BG96Interface::socket_recv(void *handle, void *data, unsigned size) {
  struct BG96_socket *socket = (struct BG96_socket *)handle;
  int32_t recv;

  // CHECK:Receive for both Client and Server Sockets same?
  recv = _BG96.recv(socket->id, (char *)data, (uint32_t)size);
  if (recv < 0) {
    // wait_ms(1);//delay of 1ms <for F4>??
    // printf(".");
    if (recv == -1)
      return NSAPI_ERROR_WOULD_BLOCK;  // send this if we want to block call
                                       // (else timeout will happen)
    else if (recv == -2)
      return NSAPI_ERROR_DEVICE_ERROR;
    else if (recv == -3)
      return NSAPI_ERROR_NO_CONNECTION;
  }
  return recv;
}

/**
 * @brief  send data to a udp socket
 * @param  handle: Pointer to handle
 *         addr: address of udp socket
 *         data: pointer to data
 *         size: size of data
 * @retval no of bytes sent
 */
/*
int BG96Interface::socket_sendto(void *handle, const SocketAddress &addr,
                                 const void *data, unsigned size) {
  struct BG96_socket *socket = (struct BG96_socket *)handle;
  if (!socket->connected) {
    int err = socket_connect(socket, addr);
    if (err < 0) {
      return err;
    }
  }

  return socket_send(socket, data, size);
}
*/
/**
 * @brief  receive data on a udp socket
 * @param  handle: Pointer to handle
 *         addr: address of udp socket
 *         data: pointer to data
 *         size: size of data
 * @retval no of bytes read
 */
/*
int BG96Interface::socket_recvfrom(void *handle, SocketAddress *addr,
                                   void *data, unsigned size) {
  struct BG96_socket *socket = (struct BG96_socket *)handle;
  return socket_recv(socket, data, size);
}
*/
/**
 * @brief  attach function/callback to the socket
 *         Not used
 * @param  handle: Pointer to handle
 *         callback: callback function pointer
 *         data: pointer to data
 * @retval none
 */
void BG96Interface::socket_attach(void *handle, void (*callback)(void *),
                                  void *data) {
  // No implementation yet
}

/**
 * @brief  utility debug function for printing to serial terminal
 * @param  string: Pointer to data
 * @retval none
 */
void BG96Interface::debug(const char *string) {
  //_BG96.debug_print(string);
}

bool BG96Interface::go2PSM() {
  return _BG96.go2PSM();
}

bool BG96Interface::creg(char * response, int length) {
  return _BG96.creg(response, length);
}

/**
* @brief  Set the socket options
*         Not used
* @param  handle: Pointer to handle
*         level:  SOL_SOCKET
*         optname: option name
*         optval:  pointer to option value
*         optlen:  option length
@retval NSAPI Error Type
*/
int BG96Interface::setsockopt(void *handle, int level, int optname,
                              const void *optval, unsigned optlen) {
  // TODO to be implemented as needed
  return NSAPI_ERROR_UNSUPPORTED;
}

/**
* @brief  Get the socket options
*         Not used
* @param  handle: Pointer to handle
*         level: SOL_SOCKET
*         optname: option name
*         optval:  pointer to option value
*         optlen:  pointer to option length
@retval NSAPI Error Type
*/
int BG96Interface::getsockopt(void *handle, int level, int optname,
                              void *optval, unsigned *optlen) {
  // TODO to be implemented as needed
  return NSAPI_ERROR_UNSUPPORTED;
}
