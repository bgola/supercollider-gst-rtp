// GstRTP.hpp
// Bruno Gola (me@bgo.la)

#pragma once

#include "gst/gst.h"
#include "gst/app/gstappsrc.h"
#include "SC_PlugIn.hpp"

static char* registryAddrs[99999];
static int registryPorts[99999];
static bool registryStatus[99999];

typedef struct _OutData {
    int channels;
    GstElement *pipeline;
    GstElement *src;
    GstElement *convert;
    GstElement *encoder;
    GstElement *pay;
    GstElement *sink;
    GstCaps *caps;
} GstRTPOutData;


typedef struct _InData {
    GstElement *pipeline;
    GstElement *udpsrc;
    GstElement *rtpopusdepay;
    GstElement *opusdec;
    GstElement *audioconvert;
    GstElement *appsink;
    GstCaps *caps;
    // GstBuffer *buffer;
    int bufIdx;
    float *dest;
    int size;
    bool allocd;
} GstInData;



namespace GstRTP {

class GstRTPOut : public SCUnit {
public:
    GstRTPOut();

    // Destructor
    ~GstRTPOut();

private:
    // Calc function
    void next(int nSamples);
    GstRTPOutData data;

    //GstBuffer *buffer;
    //GstElement *source;
    // Member variables
};


class GstIn : public SCUnit {
public:
    GstIn();

    ~GstIn();

private:
    // Calc function
    void next(int nSamples);
    //GstFlowReturn new_sample_callback(GstElement* sink, gpointer* data);

    GstInData data;
    bool get_buffer_data(int nSamples);
};


} // namespace GstRTP
