This is a quick program that controls a midi-device pedal with 5 switches and a finger-device with 3 switches (total: 8 switches)
Pressing a switch (non-latched) allows to send a midi message: NOTE-ON | Channel 1 (0x90), note 0 to 7, velocity 127 (0x7F).   

If the midi host (to which the pedal is connected to) sends midi note-on events, then the pedal can light a neopixel LED attached to it:  
* midi note_on, any note, velocity == 127 --> neopixel is set to red for 150 ms  
* midi note_on, any note, velocity >0 and <127 --> neopixel is set to yellow for 150 ms    

    
The hardware used for this is the hardware I have build for my PICOVATION project, with addition to drive neopixel LED strip.   

It is based on a tinyusb (https://github.com/hathach/tinyusb) midi example: https://github.com/hathach/tinyusb/tree/master/examples/device/midi_test   

**Install:**   
install tinyusb by git cloning  
install midi_pedal files in a folder in the tinyusb/examples/device/ folder of tinyusb (eg. /home/pi/pico/tinyusb/examples/device/midi_pedal)  

to compile, go to that folder and do as described in https://docs.tinyusb.org/en/latest/reference/getting_started.html , that is:    
$ make BOARD=raspberry_pi_pico all


**New way to compile:**  
$ # cmake  
$ cd midi_pedal  
$ mkdir build && cd build  
$ cmake -DBOARD=raspberry_pi_pico ..  
$ cd ..  
$ make  
