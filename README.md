# NbIoT prototype

The firmware for a simple device (nucleo board NUCLEO-F103RB and AVNET NBIOT-BG96-SHIELD).
The device is a prototype for testing NbIoT connection and sending GPS position. The device is most of the time in a deep sleep mode and is woken up by interrupts from RTC or accelerometer.

## SW Prerequisities

- mbed cli - see https://os.mbed.com/docs/mbed-os/v5.11/tools/developing-mbed-cli.html
- git
- BG96 with firmware version MAR02A07M1G_01.010.01.010 and higher

## How to compile

- configure mbed (TOOLCHAIN=GCC_ARM and TARGET=NUCLEO_F103RB)
- mbed deploy
- mbed compile --profile develop.profile
- mbed compile --profile develop.profile --flash

## Possible configuration parameters

The parameters can be set in __mbed_app.json__

- **DEBUG** - debug messages
- **DEBUG_GPS** - it's a fake GPS possition, not waiting for the GPS fix
- **DEBUG_NOGPS** - it's a fake __no valid__ GPS possition, not waiting for the GPS fix
- **VODAFONE** - it uses a vodafone configuration, if not set then T-Mobile (see config_default.h for more informations)

## State machine

TBD

## Communication format

The data are sent to cloud in JSON format. MQTT communication is not supported yet.

The device can also ask for a configuration from the cloud, once time per day

### JSON

Format of the json being sent to the cloud via HTTP POST

```json
  {
    "id":"ID",  
    "bat": 3.8,
    "net": 0,
    "cell":"xxx",
    "temp":0.0,
    "errFlags": 0
  }
```

- **id** - id of the device as a _string_
- **bat** - battery status (voltage) of the device in _float_
- **net** - can be just 0 or 1 where 1 means NbIoT connection, 0 GPRS
- **cell** - the cell ID of NbIoT connection as a _string_
- **temp** - a temperature of the device as a _float_ in celsius degree
- **errFlags** - error flags in bit format (_b1_ _b2_), _b1_ - if set then previous message lost, _b2_ - if set then gps was not inicialized

The configuration JSON from the cloud

```json
  {
    "gpsFixTimeout": 120,
    "accThreshold": 3,
    "wakeupTime": 10,
    "interruptTime": 30
  }
```

- **gpsFixTimeout** - how long to wait for the GPS fix (in seconds)
- **accThreshold** - accelerometer threshold to wake up the device (smaller more sensitive)
- **wakeupTime** - set the time after that the device is woken up to check if the device is in move (in minutes)
- **interruptTime** - send data anyway after this timeout (in minutes)

The device asks for the configuration via HTTP GET request and the format is with __http://test.com/$$ID$$__  where the ID is idetification of the device. The URI can be configure, __ID__ is added before the query then.
