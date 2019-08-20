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
 * @file    BG96Interface.h
 * @author  STMicroelectronics
 * @brief   Header file of the NetworkStack for the BG96 Device
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

#ifndef BG96SA_INTERFACE_H
#define BG96SA_INTERFACE_H

#include "BG96.h"

#define BG96SA_SOCKET_COUNT 8
#define SERVER_SOCKET_NO 9

enum nsapi_error {
    NSAPI_ERROR_OK                  =  0,        /*!< no error */
    NSAPI_ERROR_WOULD_BLOCK         = -3001,     /*!< no data is not available but call is non-blocking */
    NSAPI_ERROR_UNSUPPORTED         = -3002,     /*!< unsupported functionality */
    NSAPI_ERROR_PARAMETER           = -3003,     /*!< invalid configuration */
    NSAPI_ERROR_NO_CONNECTION       = -3004,     /*!< not connected to a network */
    NSAPI_ERROR_NO_SOCKET           = -3005,     /*!< socket not available for use */
    NSAPI_ERROR_NO_ADDRESS          = -3006,     /*!< IP address is not known */
    NSAPI_ERROR_NO_MEMORY           = -3007,     /*!< memory resource not available */
    NSAPI_ERROR_NO_SSID             = -3008,     /*!< ssid not found */
    NSAPI_ERROR_DNS_FAILURE         = -3009,     /*!< DNS failed to complete successfully */
    NSAPI_ERROR_DHCP_FAILURE        = -3010,     /*!< DHCP failed to complete successfully */
    NSAPI_ERROR_AUTH_FAILURE        = -3011,     /*!< connection to access point failed */
    NSAPI_ERROR_DEVICE_ERROR        = -3012,     /*!< failure interfacing with the network processor */
    NSAPI_ERROR_IN_PROGRESS         = -3013,     /*!< operation (eg connect) in progress */
    NSAPI_ERROR_ALREADY             = -3014,     /*!< operation (eg connect) already in progress */
    NSAPI_ERROR_IS_CONNECTED        = -3015,     /*!< socket is already connected */
    NSAPI_ERROR_CONNECTION_LOST     = -3016,     /*!< connection lost */
    NSAPI_ERROR_CONNECTION_TIMEOUT  = -3017,     /*!< connection timed out */
    NSAPI_ERROR_ADDRESS_IN_USE      = -3018,     /*!< Address already in use */
    NSAPI_ERROR_TIMEOUT             = -3019,     /*!< operation timed out */
    NSAPI_ERROR_BUSY                = -3020,     /*!< device is busy and cannot accept new operation */
};

typedef enum nsapi_protocol {
    NSAPI_TCP, /*!< Socket is of TCP type */
    NSAPI_UDP, /*!< Socket is of UDP type */
} nsapi_protocol_t;



/** BG96SAInterface class
 *  Implementation of the NetworkStack for the BG96 Device
 */
class BG96Interface {
 public:
  BG96Interface(PinName tx, PinName rx, bool debug);
  BG96Interface(PinName tx, PinName rx, PinName reset = PC_12,
                PinName wakeup = PC_8, bool debug = false);
  virtual ~BG96Interface();

  // Implementation of WiFiInterface
  virtual int connect();

  virtual int disconnect();
  virtual bool setGPS();
  virtual bool GPSOff();

  virtual bool setConnection(uint32_t band, char * code, const char *apn, const char *username, const char *password, bool nbiot);

  virtual int getGPS(char *gps_pointer, int array_lenght);
  virtual int getVoltage();
  virtual const char *get_mac_address();
  void debug(const char *string);

  // Implementation of NetworkStack
  virtual const char *get_ip_address();
  int init(void);
  bool go2PSM();
  void wakeup();
  void shutdown();
  bool creg(char * response, int length);

  virtual int socket_open(void **handle, nsapi_protocol_t proto);
  virtual int socket_close(void *handle);
  virtual int socket_connect(void *handle, const char * addr, int port);
  virtual bool socket_openstream(void *handle, unsigned size);
  virtual int socket_writestream(void *handle, const char * data, uint32_t amount);
  virtual bool socket_closestream(void *handle);

  virtual int socket_openreadstream(void *handle);
  virtual int socket_readstream(const char * data, uint32_t amount);
  virtual bool socket_closereadstream();
  
  virtual int socket_send(void *handle, const void *data, unsigned size);
  virtual int socket_recv(void *handle, void *data, unsigned size);

 protected:
  // Implementation of NetworkStack
  virtual int socket_listen(void *handle, int backlog);
  // virtual     nsapi_error_t socket_accept(nsapi_socket_t server,
  // nsapi_socket_t *handle, SocketAddress *address);
  virtual int socket_accept(void **handle, void *server);
/*
  virtual int socket_sendto(void *handle, const SocketAddress &address,
                            const void *data, unsigned size);
  virtual int socket_recvfrom(void *handle, SocketAddress *address,
                              void *buffer, unsigned size);
*/                              
  virtual void socket_attach(void *handle, void (*callback)(void *),
                             void *data);
  virtual int setsockopt(void *handle, int level, int optname,
                         const void *optval, unsigned optlen);
  virtual int getsockopt(void *handle, int level, int optname, void *optval,
                         unsigned *optlen);

 private:
  BG96 _BG96;
  bool _ids[BG96SA_SOCKET_COUNT];
  bool isListening;
  bool isInitialized;
};

#endif
