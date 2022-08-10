# Building on Linux

For more information, refer to "https://wiki.wxwidgets.org/Installing_and_configuring_under_Ubuntu"

## Install required packages

```
sudo apt-get install build-essential
sudo apt-get install libwxgtk3.0-dev
```

## Fetch/Build application

```
git clone git@Ubuntu-Gitlab:ADI/SAM-flasher-GUI.git
cd SAM-flasher-GUI
make
```

## Run application

```
bin/sam-flasher
```

# Building on Windows (MinGW/MSYS environment)

## Good overview

https://stackoverflow.com/questions/30069830/how-to-install-mingw-w64-and-msys2

## Install MSYS2 (http://www.msys2.org/)

Install into e:\msys64 (or any convenient drive)

```
pacman -Syu
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-wxWidgets
pacman -S make
pacman -S git
pacman -S python3
```

## Fetch/Build application

```
git clone git@Ubuntu-Gitlab:ADI/SAM-flasher-GUI.git
cd SAM-flasher-GUI
make
```

## Make installer package with all required dlls

```
make mingw-pkg

```

Output can be found in bin/mingw-pkg

## Make ISS installer

```
TODO

```

## Linux permissions

By default normal users do not have permissions to access USB serial
ports.  To resolve a `ser_open:unable to open port:permission denied
` error, either run the application using the `sudo` command or
install an appropriate `udev` rule.

The `udev` approach is better.  Once the `udev` rule is installed,
there's no need to restart the machine.  The permissions change will
take effect the next time the SAM board is plugged in or put in
boot-loader mode and will persist until the rule is removed.

Examples for both are shown below:

# sudo
```
sudo bin/sam-flasher
```

# udev
```
echo 'KERNEL=="ttyACM0", MODE="0666"' | sudo tee /etc/udev/rules.d/99-SAM-Flasher.rules > /dev/null
```
