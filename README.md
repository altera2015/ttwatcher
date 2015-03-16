# ttwatcher

Application to process .ttbin files and download files from TT Sport Watch. 

I was having issues with my TT Sport watch and needed a way to process the .ttbin 
files that the watch was giving us but were corrupted. A quick search showed the 
two projects below had done a lot of the ground work but were written 
for Linux. So I set out to make a quick and dirty Windows version so I could 
experiment with improving the ttbin parsing / communications.

**DISCLAIMER: I STRONGLY SUGGEST USING THE OFFICIAL APP TO DOWNLOAD YOUR ttbin 
files.**

# Releases

###1.3.0 March 15, 2015 Fixed installer + Usability
[Download 1.3.0 Installer Here](https://github.com/altera2015/ttwatcher/releases/download/v1.3.0/TTWatcherSetup_x86_1.3.0.0.exe)
* Fixed installer ( was missing some Qt files)
* Fixed TCX export to round to 9 digits instead of 6
* Added drag and drop support

###1.2.0 March 11, 2015 Fixed Running Cadence
[Download 1.2.0 Installer Here](https://github.com/altera2015/ttwatcher/releases/download/v1.2.0/TTWatcherSetup_x86_1.2.0.0.exe)

###1.1.0 March 11, 2015 Cleanup and Feature updates
[Download 1.1.0 Installer Here](https://github.com/altera2015/ttwatcher/releases/download/v1.1.0/TTWatcherSetup_x86_1.1.0.0.exe)

* Added support for Cadence exporting (even during Running activity!)
* Added support for Elevation Data
* Improved Graphs
* Lots of cleanups
* Added installer

###1.0.0 March 10, 2015 Initial Release

# Heritage

Original inspiration from:

* https://github.com/FluffyKaon/TomTom-ttbin-file-format
* https://github.com/ryanbinns/ttwatch

# Attributions

* Running man Icon made by http://www.freepik.com" is licensed under Creative Commons BY 3.0 CC BY 3.0
* QCustomPlot, an easy to use, modern plotting widget for Qt, Copyright (C) 2011, 2012, 2013, 2014 Emanuel Eichhammer http://www.qcustomplot.com/
* Built with Qt 5.4 LGPL http://www.qt-project.org
* qhttpserver from https://github.com/nikhilm/qhttpserver

# Building

Install Qt 5.x Framework and load up the ttwatcher.pro file in the src directory. On Windows you might have to copy the hid.lib to your Release or Debug output directory if you get linking errors.
