# ttwatcher

Application to process .ttbin files and download files from TT Sport Watch. 

I was having issues with my TT Sport watch and needed a way to process the .ttbin 
files that the watch was giving us but were corrupted. A quick search showed the 
two projects below had done a lot of the ground work but were written 
for Linux. So I set out to make a quick and dirty Windows version so I could 
experiment with improving the ttbin parsing / communications.

![](https://raw.githubusercontent.com/altera2015/ttwatcher/master/screenshot1.jpg)

# Current Releases

You can find all releases here
https://github.com/altera2015/ttwatcher/releases

# Documentation

The [Wiki](https://github.com/altera2015/ttwatcher/wiki) has some information on how to use the application.

# Heritage

Original inspiration from:

* https://github.com/FluffyKaon/TomTom-ttbin-file-format
* https://github.com/ryanbinns/ttwatch

# Attributions

* Icons made by 
	* [Freepik](http://www.flaticon.com/authors/freepik)
	* [OCHA](http://www.flaticon.com/authors/ocha)
	* [Icomoon](http://www.flaticon.com/authors/icomoon)
	* [Elegant Themes](http://www.flaticon.com/authors/elegant-themes)
	* [Yannick](http://www.flaticon.com/authors/yannick)
	* [Amit Jakhu](http://www.flaticon.com/authors/amit-jakhu)
	
  Icons were modified to add a touch of color, all are licensed under Creative Commons BY 3.0 CC BY 3.0
  
  
* QCustomPlot, an easy to use, modern plotting widget for Qt, Copyright (C) 2011, 2012, 2013, 2014 Emanuel Eichhammer http://www.qcustomplot.com/
* Built with Qt 5.4 LGPL http://www.qt-project.org
* qhttpserver from https://github.com/nikhilm/qhttpserver
* QuaZIP from http://quazip.sourceforge.net/
* Elevation data from NED database http://nationalmap.gov/elevation.html
* Elevation data from SRTM NASA Shuttle missions http://www2.jpl.nasa.gov/srtm/

# Building

Install Qt 5.x Framework and load up the TTWatcherMain.pro file in the src directory.

## Windows
You might have to copy the hid.lib to your Release or Debug output directory if you get linking errors.

## Linux

Tested on Ubuntu 14.04, 14.10 and 15.04.

On Linux if your app does not run as root you'll need to have permission to access the hid device. The easiest way is this:
```
$ cat /etc/udev/rules.d/99-tomtom.rules
SUBSYSTEMS=="usb", ATTRS{idVendor}=="1390", ATTRS{idProduct}=="7474", MODE="666"
```
More discerning users might setup a special user group and only allow that user group access. 


```
$ sudo apt-get install qtbase5-dev qt5-qmake

# enable QT5 if also QT4 is installed
$ export QTCHOOSER_RUNTOOL=qtconfig
$ export QT_SELECT=5

$ qmake -v
QMake version 3.0
Using Qt version 5.2.1 in /usr/lib/x86_64-linux-gnu

# prepare export credentials
$ cd ttwatcher/src
$ cp strava_auth.h.orig strava_auth.h
$ cp runkeeper_auth.h.orig runkeeper_auth.h
```

Build from CLI:

```
$ cd ..
$ mkdir build && cd build
$ qmake ../TTWatcherMain.pro 
$ make
$ sudo make install
$ sudo install -m 755 -p src/ttwatcher /usr/local/bin
$ ttwatcher

```

Build using GUI IDE:

```
$ sudo apt-get install qtcreator
$ qtcreator
```
Import project by opening the TTWatcherMain.pro file.

