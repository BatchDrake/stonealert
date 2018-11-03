# Warning!
This project is a mere PoC of an automatic GRAVES echo detector and recorder, based on another PoC I've written named [meteodect](https://github.com/BatchDrake/meteodetect). It will probably stay in unmaintained state forever unless bugs or strong optimizations are found. However, the code in `graves.c` / `graves.h` may inspire a hypothetical, well-designed automatic GRAVES echo detection system, probably integrated into something more interesting than a tacky SDL interface.

Use it under your own risk!

## How to use it
Compile it as usual:

```
% autoreconf -fvi
% ./configure
% make
% sudo make install
```

`stonealert` will look for echoes in exactly 1000 Hz, and therefore you will need to tune your receiver to 143.049 MHz in order to make it work. The frequency must be **exact**, as it is used to compute Doppler shifts.


### Detect echoes from the soundcard
Just run it without argments
```
% stonealert
```

A file named `capture_<timestamp>_8000sps.raw` will be created in the working directory. This is a regular 32-bit float IQ capture file of sample rate 8000 sps that will keep all the received data.

### Detect echoes from a capture file
A capture file must be a 32-bit float IQ of sample rate 8000 sps (like the ones produced by gqrx or saved by this program when listening from the soundcard). Just pass it as a command line argument:

```
% stonealert capture_<timestamp>_8000sps.raw
```

## Analizing the data
After a successful run of `stonealert`, a text file named `chirps_<timestamp>.dat` is created. It will contain one line for each received echo. Lines follow the format:

```
TIMESTAMP;SNR;DOPPLER SHIFT;SAMPLES
```

Where `TIMESTAMP` is the Unix timestamp of the first sample of the echo, `SNR` the Signal-to-noise ratio of the echo in dBs, `DOPPLER SHIFT` the Doppler shift of the echo expressed in meters per second and `SAMPLES` a comma-separated list of complex samples at 8000 sps of the echo, centered around 0 Hz.

