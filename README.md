# GstRTP

A SuperCollider plugin to send audio via RTP using GStreamer directly from SuperCollider

### Requirements

- CMake >= 3.5
- SuperCollider source code
- gstreamer-1.0
- gstreamer-app-1.0
- gstreamer-plugins-base

### Building

Clone the project:

    git clone https://github.com/bgola/gst
    cd gst
    mkdir build
    cd build

Then, use CMake to configure and build it:

    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release
    cmake --build . --config Release --target install

You may want to manually specify the install location in the first step to point it at your
SuperCollider extensions directory: add the option `-DCMAKE_INSTALL_PREFIX=/path/to/extensions`.

It's expected that the SuperCollider repo is cloned at `../supercollider` relative to this repo. If
it's not: add the option `-DSC_PATH=/path/to/sc/source`.
