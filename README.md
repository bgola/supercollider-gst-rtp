# GstRTP

A SuperCollider plugin to send audio via RTP using GStreamer directly from SuperCollider

## Example

```supercollider
// Mono signal

GstRTP.addOut(\gstoutmono, '127.0.0.1', 9900);
~bus = Bus.audio(s, 1);
Ndef(\outbus, {GstRTPOut.ar(\gstoutmono, InFeedback.ar(~bus))});
Ndef(\send, {Out.ar(~bus, WhiteNoise.ar)});

// also works with stereo signal
GstRTP.addOut(\gstoutstereo, '127.0.0.1', 9900);
~busstereo = Bus.audio(s, 2);
Ndef(\outbusstereo, {GstRTPOut.ar(\gstoutstereo, InFeedback.ar(~busstereo, 2))});
Ndef(\sendstereo, {Out.ar(~busstereo, WhiteNoise.ar*LFNoise2.ar(2!2))});
```

You can check if it is working with the following pipeline on Linux with Pipewire:

```sh
gst-launch-1.0 udpsrc port=9900 ! application/x-rtp, media=audio, encoding-name=OPUS, sprop-stereo=0, payload=96  !  rtpopusdepay ! opusdec  ! audioconvert ! pipewiresink client-name="gstrst"
```

or with Jack:

```sh
gst-launch-1.0 udpsrc port=9900 ! application/x-rtp, media=audio, encoding-name=OPUS, sprop-stereo=0, payload=96  !  rtpopusdepay ! opusdec  ! audioconvert ! jacksink name="gstrst"
```


### Requirements

- CMake >= 3.12
- SuperCollider source code
- gstreamer-1.0
- gstreamer-app-1.0
- gstreamer-plugins-base

### Building

Clone the project:

    git clone https://github.com/bgola/supercollider-gst-rtp
    cd supercollider-gst-rtp
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
