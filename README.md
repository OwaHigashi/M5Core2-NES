# M5Core2 NES Emulator on ESP-IDF

# Build Instruction

~~~
$ git clone -b v4.3.4 --recursive https://github.com/espressif/esp-idf.git esp-idf-v4.3.4
$ cd esp-idf-v4.3.4/
$ ./install.sh  # Only once
$ . export.sh
$ cd ..
$ git clone --recursive https://github.com/HidetoKimura/m5core2_esp-idf_nesemu.git
$ cd m5core2_esp-idf_nesemu
$ ./flash.sh  # Write NES rom. Only once.
$ idf.py build flash monitor
~~~
