#！/bin/bash

esptool.py --port /dev/ttyUSB0 -b 115200 write_flash 0x0000 ../bin/boot_v1.7.bin 0x1000 ../bin/upgrade/user1.2048.new.3.bin 0x1fc000 ../bin/esp_init_data_default_v05.bin 0x1fe000 ../bin/blank.bin