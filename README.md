wallpaperd
==========

wallpaperd is a small application that takes care of setting the
background image. wallpaperd was created due to a friend requesting to
have different wallpapers on the different workspaces in
[pekwm](https://github.com/pekdon/pekwm).

Features
--------

wallpaperd now supports:

* Changing wallpaper on workspace change.
* Changing wallpaper every X amount of time.
* Changing wallpaper based on a GNOME background.xml file.
* Support for specifying centered, zoomed, tiled and fill image modes.
* Selecting wallpaper based on workspace number.
* Selecting wallpaper based on workspace name.
* RANDR support setting the wallpaper on each screen.
* RANDR support re-setting the wallpaper on screen resolution changes.
* Setting background Atom hint.

plans for implementing support for:

* RANDR support having per screen wallpapers.

Supported platforms
-------------------

wallpaperd is known to compile and run on the following platforms:

* NetBSD 9.1

wallpaperd has been known to compile and run on the following
platforms:

* Ubuntu 10.04
* OpenBSD 4.6
* OS X 10.6
* OpenSolaris 2009.06, both with GCC and Sun Studio compiler. Requires
  SUNWgnome-common-devel and SUNWxorg-headers packages. Imlib2 is not included
  in the standard package repository.

Installation
------------

wallpaperd requires:

* CMake
* Xlib, X11 development files.
* Imlib2, Image loading and manipulation

To install (download, extract, configure, compile and install) execute:

```
$ curl -O https://github.com/pekdon/wallpaperd/releases/download/release-0.2.3/wallpaperd-0.2.3.tar.gz
$ tar xvzf wallpaperd-0.2.3.tar.gz
$ cd wallpaperd-0.2.3
$ mkdir build
$ cd build
$ cmake ..
$ make
$ sudo make install
```

Configuration
-------------

wallpaperd is configured in _~/.wallpaperd.cfg_ which is a simple
section free ini style configuration file.

An example configuration file setting the wallpaper based on workspace
name, having special wallpapers for work and www workspaces and a default
image named default.jpg:

```
path.search=~/Pictures:~/Pictures/Wallpapers:/usr/share/backgrounds
config.mode=NAME
wallpaper.default.image=default.jpg
wallpaper.default.mode=CENTERED
wallpaper.work.image=work.jpg
wallpaper.work.mode=ZOOMED
wallpaper.www.image=www.jpg
wallpaper.www.mode=TILED
```
