msp430_usb_jtag README.TXT

TODO:
* See TODOs in main.c
* Add documentation

NOTE:
Sometimes there are problems for the MCU to communicate over I2C with the 
TPS65217. A couple of reasons have been found:

* It is likely (but not proven) that the problems are caused by reseting
  the MCU (for downloading new code, etc) during I2C communication. 
  This causes the I2C slave to stop in a state where it does not expect
  a new message to start. That makes it impossible to communicate over I2C.
  A function in swi2cmst.c has been added, called swi2cmst_clrbus(), 
  that sends ten I2C STOPs to try to recover from this situation.
  There is no guarantee that this works, though.

* Sometimes the I2C communication between the OLIMEXINO board and 
  the ARM SoC board doesn't work, caused by incorrect insertion of 
  the SoDIMM module in the socket. It is very picky. Try to power down, 
  remove the module, reinsert it carefully, and power up.

* I get the feeling that sometimes the communication doesn't work for a reason
  which is not one of the above. It remains to find out what that reason is.

/Per


Note: Depends on libxsvf for parsing XSVF files. This comes from an external
repository. To download it's sources please run

   git submodule update --init

