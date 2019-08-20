#ifndef CONFIG_DEFAULT_H
#define CONFIG_DEFAULT_H
/**
 * 
 */

#define LASTSEND_TIMESTAMP_REG REG1
#define INMOTION_CHECK_TIME REG2
#define STATE_REG REG3
#define CONFIG_TIMESTAMP_REG  REG4
#define CONFIG_BITS  REG5

/**
 * Default configuration values
 */ 
// #define SLEEP_MS  (5*60*1000)
#define WAKEUP_INTERVAL_MIN_DEFAULT  (10)
#define SEND_INTERVAL_MIN_DEFAULT (30)
#define ACC_THRESHOLD_DEFAULT 0x03
#define GPS_FIX_TIMEOUT_DEFAULT 120
#define RESET_WD 20 // reset if the timer is still running

#define CONFIG_CHECK_S (24 * 60 * 60)  // check config timeout in seconds

#ifdef VODAFONE
#define APN_VGESACE_NB_IOT "vgesace.nb.iot"  //VODAFONE NB IoT apn definition's
#define APN_DEVAPN_GDSP "devapn.gdsp"       //VODAFONE GSM apn definition's
#define GSM_PWD ""
#define GSM_USNAME  ""
#define NETWORK_CODE "26202"
#define NBIOT_BAND 80000
#else
#define APN_VGESACE_NB_IOT "internet.nbiot.telekom.de"  //VODAFONE NB IoT apn definition's
#define APN_DEVAPN_GDSP "internet.m2mportal.de"       //VODAFONE GSM apn definition's
#define GSM_PWD "sim"
#define GSM_USNAME  "m2m"
#define NETWORK_CODE "26201"
#define NBIOT_BAND 80
#endif

#ifdef MQTT
#define MQTT_ID_PREFIX "d"
#define MQTT_ORG "quickstart"
#define MQTT_BROKER_ADDR MQTT_ORG ".messaging.internetofthings.ibmcloud.com"
#define MQTT_BROKER_URL ".messaging.internetofthings.ibmcloud.com"
#define MQTT_ID  ""
#define MQTT_AUTH_TOKEN ""
#define MQTT_DEFAULT_TYPE_NAME "iotsample-mbed-Nucleo"
#define MQTT_CLIENT_ID  MQTT_ID_PREFIX ":" MQTT_ORG ":" MQTT_DEFAULT_TYPE_NAME ":" MQTT_ID
#define MQTT_TOPIC  "iot-2/evt/status/fmt/json"
#define MQTT_PORT 1883
#define MQTT_TLS_PORT 8883

#else

#define HTTP_PORT 80
#define HTTP_REST_API_HOSTADDR_DEBUG "yourdebugaddr"
#define HTTP_REST_API_URL_DEBUG "/cgi-bin/test.py"
#define HTTP_REST_API_URL_CONFIG_DEBUG "/config.txt?id="
#define HTTP_REST_API_HOSTADDR_PROD "yourprodaddr"
#define HTTP_REST_API_URL_PROD "/cgi-bin/test.py"
#define HTTP_REST_API_URL_CONFIG_PROD "/config.txt?id="
#ifdef DEBUG_URL
#define HTTP_REST_API_URL HTTP_REST_API_URL_DEBUG
#define HTTP_REST_API_HOSTADDR HTTP_REST_API_HOSTADDR_DEBUG
#define HTTP_REST_API_URL_CONFIG HTTP_REST_API_URL_CONFIG_DEBUG
#else
#define HTTP_REST_API_URL HTTP_REST_API_URL_PROD
#define HTTP_REST_API_HOSTADDR HTTP_REST_API_HOSTADDR_PROD
#define HTTP_REST_API_URL_CONFIG HTTP_REST_API_URL_CONFIG_PROD
#endif // DEBUG_URL

#endif // MQTT

#endif // CONFIG_DEFAULT_H



