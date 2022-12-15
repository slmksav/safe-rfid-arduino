# servo-safe-arduino
An embedded project made during first semester studies at Oulu University of Applied Sciences.

This Arduino project uses an RFID reader, a servo motor, a buzzer, an LCD display, and a 3x4 keypad to create a lock and key system that is able to store a password and two slots for RC-522-compatible tags (the reader uses a 13.56 MHz electromagnetic field) into the microcontrollers EEPROM-memory and work with them during runtime.

It also includes functions for reading and storing RFID tags, as well as for controlling the servo motor and displaying information such as time on the LCD and indicating states of the device with light and sound. The system can be switched between RFID and keypad modes using a button.
