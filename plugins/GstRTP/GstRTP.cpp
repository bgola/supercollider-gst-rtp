// GstRTP.cpp
// Bruno Gola (me@bgo.la)

#include "SC_PlugIn.hpp"
#include "GstRTP.hpp"
#include "gst/gst.h"
#include "gst/app/gstappsrc.h"
#include "gst/app/gstappsink.h"
#include <stdio.h>
#include <stdlib.h>
#include "glib.h"


static InterfaceTable* ft;

namespace GstRTP {

GstRTPOut::GstRTPOut() {
    mCalcFunc = make_calc_function<GstRTPOut, &GstRTPOut::next>();

    const float *chans = in(0);
    const float *id = in(1);
    int channels = *chans;
    int key = *id;
    if(!registryStatus[key]) {
        Print("Warning: key %d was not initialized.\n", key);
        return;
    };

    if(channels != 1 && channels != 2) {
        Print("GstRTPOut input should be mono or stereo\n");
        return;
    };

    char *addr = registryAddrs[key];
    int port = registryPorts[key];
    data.channels = channels;
    data.pipeline = gst_pipeline_new("audio-pipeline");
    data.src = gst_element_factory_make("appsrc", "audio-source");
    data.convert = gst_element_factory_make("audioconvert", "convert");
    data.encoder = gst_element_factory_make("opusenc", "encoder");
    g_object_set(G_OBJECT(data.encoder), "bitrate", 64000, NULL);
    data.pay = gst_element_factory_make("rtpopuspay", "pay");
    data.sink = gst_element_factory_make("udpsink", "sink");
    
    g_object_set(G_OBJECT(data.sink), "host", addr, "port", port, NULL);
    
    g_object_set(G_OBJECT(data.src), "is-live", true, "emit-signals", false, "format", GST_FORMAT_TIME, "stream-type", GST_APP_STREAM_TYPE_STREAM, NULL);
    g_object_set(G_OBJECT(data.src), "caps",
                 gst_caps_new_simple("audio/x-raw",
                                     "format", G_TYPE_STRING, "F32LE",
                                     "layout", G_TYPE_STRING, "interleaved",
                                     "channels", G_TYPE_INT, data.channels,
                                     //"channel-mask", GST_TYPE_BITMASK, 0x1,
                                     "rate", G_TYPE_INT, (int)sampleRate(),
                                     NULL), NULL);
    
    // Create the caps filter
    data.caps = gst_caps_new_simple("audio/x-raw",
                               "channels", G_TYPE_INT, data.channels,
                               NULL);
    
    // Build the pipeline
    gst_bin_add_many(GST_BIN(data.pipeline), data.src, data.convert, data.encoder, data.pay, data.sink, NULL);
    if (!gst_element_link_filtered(data.src, data.convert, data.caps) ||
        !gst_element_link(data.convert, data.encoder) ||
        !gst_element_link(data.encoder, data.pay) ||
        !gst_element_link(data.pay, data.sink)) {
        g_printerr("Failed to link elements in the pipeline\n");
        gst_object_unref(data.pipeline);
        return;
    }

    // Start playing
    gst_element_set_state(data.pipeline, GST_STATE_PLAYING);

    next(1);
}

GstRTPOut::~GstRTPOut() {
    gst_object_unref(data.pipeline);
}

void GstRTPOut::next(int nSamples) {
    const float* input = in(2);
    const float* input_right = in(3);
    
    float* outbuf = out(0);
    Unit* unit = (Unit*) this;
    
    GstBuffer *buf = gst_buffer_new_allocate(NULL, nSamples * sizeof(float) * data.channels, NULL);
    gst_buffer_memset(buf, 0, 0, nSamples * data.channels * sizeof(float));
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_WRITE);

    float *samples = (float *)map.data;
    for (int i=0; i < nSamples; ++i) {
        samples[i * data.channels] = input[i];
        outbuf[i] = 0.0;
        if (data.channels == 2) { 
            samples[i * data.channels + 1] = input_right[i]; 
        };
    };

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(data.src), buf);
    if (ret != GST_FLOW_OK) {
        Print("Failed to push buffer to appsrc\n");
    }
    gst_buffer_unmap(buf, &map);
}



GstIn::GstIn() {
    mCalcFunc = make_calc_function<GstIn, &GstIn::next>();

    const float *chans = in(0);
    const float *id = in(1);
    int channels = *chans;
    int key = *id;
    if(!registryStatus[key]) {
        Print("Warning: key %d was not initialized.\n", key);
        return;
    };

    if(channels != 1 && channels != 2) {
        Print("GstRTPOut input should be mono or stereo\n");
        return;
    };

    char *addr = registryAddrs[key];
    int port = registryPorts[key];
 
    data.allocd = false;
    // Create the main pipeline
    data.pipeline = gst_pipeline_new("audio-pipeline");

    // Create elements
    data.udpsrc = gst_element_factory_make("udpsrc", "udp-source");
    data.rtpopusdepay = gst_element_factory_make("rtpopusdepay", "rtp-opus-depay");
    data.opusdec = gst_element_factory_make("opusdec", "opus-decoder");
    data.audioconvert = gst_element_factory_make("audioconvert", "audio-converter");
    data.appsink = gst_element_factory_make("appsink", "app-sink");
    //data.appsink = gst_element_factory_make("pipewiresink", "pipewire-sink");

    // Create caps for RTP Opus depayloader
    GstCaps* caps = gst_caps_new_simple("application/x-rtp",
        "media", G_TYPE_STRING, "audio",
        "encoding-name", G_TYPE_STRING, "OPUS",
        "sprop-stereo", G_TYPE_INT, 0,
        "payload", G_TYPE_INT, 96,
        NULL);

    // Set properties
    g_object_set(G_OBJECT(data.udpsrc), "port", port, NULL);
    g_object_set(G_OBJECT(data.appsink), "caps",
                 gst_caps_new_simple("audio/x-raw",
                                     "format", G_TYPE_STRING, "F32LE",
                                     "layout", G_TYPE_STRING, "interleaved",
                                     "channels", G_TYPE_INT, 1,
                                     "channel-mask", GST_TYPE_BITMASK, 0x1,
                                     "rate", G_TYPE_INT, (int)sampleRate(),
                                     NULL), NULL);

    // Add elements to the pipeline
    gst_bin_add_many(GST_BIN(data.pipeline), data.udpsrc, data.rtpopusdepay, data.opusdec, data.audioconvert, data.appsink, NULL);
   
	// Link elements
    if (!gst_element_link_filtered(data.udpsrc, data.rtpopusdepay, caps)) {
        g_printerr("Failed to link udpsrc and rtpopusdepay");
    }
    if (!gst_element_link(data.rtpopusdepay, data.opusdec) ||
        !gst_element_link(data.opusdec, data.audioconvert) ||
        !gst_element_link(data.audioconvert, data.appsink)) {
        g_printerr("Failed to link elements");
    }

	//g_signal_connect(data.appsink, "new-sample", G_CALLBACK(new_sample_callback), this);

    // Start the pipeline
    GstStateChangeReturn ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to start pipeline");
        //return ;
    }

    //data.buffer = gst_buffer_new_allocate(NULL, 64 * sizeof(float), NULL);
    //if(data.buffer == NULL) {
    //    Print("FAILED TO ALLOC BUFFER\n");
    //}
    Print("Done\n");

    next(1);
}

GstIn::~GstIn() {
	if (data.allocd) { 
        //RTFree(unit->mWorld, (gpointer*)data.dest); 
        data.allocd = false;
    }
    gst_buffer_unref(data.buffer);
    gst_object_unref(data.pipeline);
}

bool GstIn::get_buffer_data(int nSamples) {
    GstSample* sample = gst_app_sink_try_pull_sample(GST_APP_SINK(data.appsink), 10);
    Unit* unit = (Unit*) this;
    if (sample) {
        if (data.allocd && data.bufIdx < ((data.size/sizeof(float))-nSamples)) {
            Print("Warning: got sample while still reading old buffer, bufidx: %d!!\n", data.bufIdx);
        } else {
            GstBuffer* buffer = gst_sample_get_buffer(sample);
            data.size = gst_buffer_get_size(buffer);

            if(data.allocd) { 
                RTFree(unit->mWorld, (gpointer*)data.dest);
                data.allocd = false;
            };
           
            data.dest = (float*) RTAlloc(unit->mWorld, data.size);
            data.allocd = true;
            data.bufIdx = 0;
            
            gst_buffer_extract(buffer, 0, (gpointer*)data.dest, data.size);
            //gst_buffer_unmap(buffer, &map);
            gst_sample_unref(sample);
            return true;
        }
    }
    return false;
}

void GstIn::next(int nSamples) {
    float* outbuf = out(0);

    bool noSamples = false;
    for (int i = 0; i < nSamples; ++i) {
        if (data.allocd && data.bufIdx < (data.size/sizeof(float))) {
            outbuf[i] = data.dest[data.bufIdx];
            data.bufIdx++;
        } else {
            if (!get_buffer_data(nSamples)) {
                noSamples = true;
                if (i > 0) {
                    outbuf[i] = outbuf[i-1];
                } else {
                    outbuf[i] = 0.0;
                }
            } else {
                i = i - 1;
            }
        }
    }

    //if(noSamples) { Print("Warning: no data to read from gstreamer.\n"); } else { Print("Got samples\n"); };
}


} // namespace GstRTP



void defineOutNetAddr(World *inWorld, void* inUserData, struct sc_msg_iter *args, void *replyAddr) {
    int key = args->geti(0);
    const char *addr = args->gets("");
    int port = args->geti(9999);
    registryAddrs[key] = (char*)malloc(strlen(addr) + 1);
    strcpy(registryAddrs[key], addr);
    registryPorts[key] = port;
    registryStatus[key] = true;
    Print("%s:%d\n", registryAddrs[key], registryPorts[key]);
    //Print("got define net out %s %s %d\n", key, addr, port);
}

void defineInNetAddr(World *inWorld, void* inUserData, struct sc_msg_iter *args, void *replyAddr) {
	Print("Test!\n");
    int key = args->geti(0);
    const char *addr = args->gets("");
    int port = args->geti(9999);
    registryAddrs[key] = (char*)malloc(strlen(addr) + 1);
    strcpy(registryAddrs[key], addr);
    registryPorts[key] = port;
    registryStatus[key] = true;
    Print("%s:%d\n", registryAddrs[key], registryPorts[key]);
    //Print("got define net out %s %s %d\n", key, addr, port);
}

//void defineInNetAddr(World *inWorld, void* inUserData, struct sc_msg_iter *args, void *replyAddr) {}

PluginLoad(GstRTPUGens) {
    // Plugin magic
    ft = inTable;
    registerUnit<GstRTP::GstRTPOut>(ft, "GstRTPOut", false);
    registerUnit<GstRTP::GstIn>(ft, "GstRTPIn", false);
    DefinePlugInCmd("/gstrtp_set_out", defineOutNetAddr, nullptr);
    DefinePlugInCmd("/gstrtp_set_in", defineInNetAddr, nullptr);
    gst_init(0, NULL);
}
