I developed this UN*X library back in 2005 when I was using the Timex Data Link USB watch to transfer data between my laptop and watch. I do not maintain this library anymore since my watch broke a while ago. There were two projects forked from this library: [wxDatalinkUSB](http://wxdlusb.sourceforge.net) (wxWidgets GUI for this library) and [libdlusb-0.0.9-mac
](https://github.com/trishume/libdlusb-0.0.9-mac) (a port for Mac OS). This Git repository was imported from the latest version in my local CVS repository for archiving purposes.


$Id: README,v 1.11 2005/11/21 06:41:21 geni Exp $

# libdlusb

libdlusb is a library used to communicate with the Timex Data Link USB watch on
various UN*X operating systems.  The library can be built with two USB
libraries: libusb (OS-independent USB library) and usbhid (USB HID library for
*BSD).

The communication protocol was implemented based on "Data Link USB
Communication Protocol and Database Design Guide," which is distributed by
Timex Corporation.

Homepage: https://oldgeni.isnew.info/libdlusb.html


## DISCLAIMER

This software can cause severe damage to your Timex Data Link USB watch.  Use
at your own risk.

Be prepared to do hard-reset!


## COMPILATION

### libusb

1. Create Makefile.  The configure script generates Makefile for libusb support
   by default:
   ```
   ./configure --prefix=$HOME
   ```
2. Build the library and test programs:
   ```
   make
   ```
3. Install programs under $HOME/bin (${prefix}/bin):
   ```
   make install
   ```
4. Edit $HOME/.dlusbrc

Now you have libdlusb.a and several test programs.  If you want a shared
library, add `--enable-shared` option to configure (`./configure --enable-shared`)
and run make.

NOTE: Under *BSD, install the libusb port from /usr/ports/devel/libusb and
unload the uhid kernel module or disable it from the kernel config.


### usbhid

This applies only to *BSD.

1. As root, you need to add the following lines into the kernel config
   (/usr/src/sys/i386/conf/MYKERNEL) to support USB and HID:
   ```
   device	uhci
   device	ohci
   device	ehci
   device	usb
   device	ugen
   device	uhid
   ```
2. Recompile the kernel and reboot:
   ```
   cd /usr/src
   make kernel KERNCONF=MYKERNEL
   reboot
   ```
3. Configure libdlusb with usbhid support:
   ```
   ./configure --with-usbhid --prefix=$HOME
   ```
4. Build the library and test programs:
   ```
   make
   ```
5. Install programs under $HOME/bin (${prefix}/bin):
   ```
   make install
   ```
6. Edit $HOME/.dlusbrc

7. Use /dev/uhid0 for the device file.  If it doesn't work, you can see which
   file is linked with the watch by tailing /var/log/message while
   attaching/detaching the watch (`tail -f /var/log/message`).

That's it!  Add `--enable-shared` option to configure (`./configure --with-usbhid
--enable-shared`) to build a shared library.

NOTE: When you switch between the two USB libraries, make sure to delete an old
library (`make clean; rm -f libdlusb.*; ./configure ...`).


## BUG REPORT

If you find a bug or write a useful program with the library, please let me
know: http://groups.yahoo.com/group/timexdatalinkusbdevelop
