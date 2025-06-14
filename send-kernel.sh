#!/usr/bin/ckermit

set port /dev/ttyUSB0
set speed 921600
set carrier-watch off
set flow-control none
set prefixing all

send ./kernel.bin
