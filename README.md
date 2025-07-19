This is a quick program that controls a pedal with 5 switches
Pressing a switch (non-latched) allows to send a midi message: NOTE-ON | Channel 1 (0x90), note 0 to 4, velocity 127 (0x7F).   

The hardware used for this is the hardware I have build for my PICOVATION project.   

It is based on a tinyusb (https://github.com/hathach/tinyusb) midi example: https://github.com/hathach/tinyusb/tree/master/examples/device/midi_test   

Install:   
install tinyusb by git cloning  
install midi_pedal files in a folder in the tinyusb/examples/device/ folder of tinyusb (eg. /home/pi/pico/tinyusb/examples/device/midi_pedal)  

to compile, go to that folder and do as described in https://docs.tinyusb.org/en/latest/reference/getting_started.html , that is:   
$ make BOARD=raspberry_pi_pico all


New way to compile:
$ # cmake
$ mkdir build && cd build
$ cmake -DBOARD=raspberry_pi_pico ..
$ cd ..
$ make
