PropellerIDE
============

The new home of the Propeller IDE.

### Checkout

```
git clone https://github.com/lamestation/PropellerIDE.git PropellerIDE
cd PropellerIDE
git submodule init
git submodule update
```

### Setting Up The Build Environment

```
sudo apt-get install qtcreator qt5-default devscripts zlib1g-dev nsis
```

### Building

Type `make deb` to build a Debian package.

```
$ make deb
```

Type `make clean` to remove old build files. Open the makefile to view other targets available.

```
make clean
```
