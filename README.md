# minirig_arduino

Arduino IDE code

Preparing MATLAB:

1.  Make sure you have the minirig_comms class
2.  On Ubuntu machine (current for 14.04) make sure that MATLAB can find `/dev/ttyAMC0` by creating a `java.opts` file in either your home directory or matlabroot / bin / arch for example `/usr/local/MATLAB/R2015b/bin/glnxa64` and make sure it has the following line: `-Dgnu.io.rxtx.SerialPorts=/dev/ttyS0:/dev/ttyS1:/dev/USB0:/dev/ttyACM0`
3.  Alternatively you could add a symbolic link `/dev/ttyS101` that points to `/dev/ttyAMC0` (I think this would work but haven't tried)
