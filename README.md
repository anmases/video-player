# Video player.
This is a basic cross-platform video player using FFmpeg to process video codecs and OpenGL to display windows.
## Add submodules:
```bash
git submodule init
git submodule update
```
## Build
Compile FFmpeg:
install necessary tools:
```bash
sudo apt-get install build-essential yasm nasm libx264-dev libx265-dev
```
configure and compile:
```bash
cd lib/FFmpeg
./configure --enable-static --disable-shared --enable-gpl --enable-libx264 --enable-libx265
make -j8
```

create build directory
generate makefile inside build directory:
```bash
cmake -G "MinGW Makefiles" .. -DCMAKE_TOOLCHAIN_FILE="C:/Users/antonio/Development/vcpkg/scripts/buildsystems/vcpkg.cmake"
```
then, execute:
```bash
make
```
## Run
Run video-player.exe in build directory.