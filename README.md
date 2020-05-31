mousr-qt-controller
===================

![screenshot](/screenshot.jpg)
![screenshot of scanning](/connecting.jpg)

Very simple (for now) Qt based controller for the Mousr cat toy, because the
official app is ... lacking.

And I wanted a desktop application because I can't always be bothered to pull
up my phone.

So far it shows the Mousr orientation and other basic state (battery, if it is
charging, if the sensor is dirty, etc.), and allows you to request it to chirp
so you can find it.

TODO
====

 * Port to BluezQt, qtbluetooth is utterly broken on Linux: https://api.kde.org/frameworks/bluez-qt/html/index.html
 * Configuring autoplay.
 * Manual control.


Work in progress
================

 * Connecting to and controlling sphero droids, because my cat also plays with those.
