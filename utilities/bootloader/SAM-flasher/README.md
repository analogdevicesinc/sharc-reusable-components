# Building on Linux

## Fetch/Build application

```
git clone git@Ubuntu-Gitlab:ADI/SAM-flasher.git
cd SAM-flasher
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
pacman -S make
pacman -S git
```

## Fetch/Build application

```
git clone git@Ubuntu-Gitlab:ADI/SAM-flasher.git
cd SAM-flasher
make
```

## Run application

```
bin/sam-flasher
```

# Linux permissions

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
