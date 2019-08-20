# NbIoT prototype

The firmware for a simple device (nucleo board NUCLEO-F103RB and AVNET NBIOT-BG96-SHIELD)

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




