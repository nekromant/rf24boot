avrdude -c buspirate -P /dev/ttyUSB0 -p m328p -U lfuse:w:0xe2:m -U hfuse:w:0xd0:m
