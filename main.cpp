#include <gst/gst.h>

#include <string>
#include <iostream>

// The following test code is implemented using GStreamer 1.26.0

enum Test {
    None,
    Webcam,
    RTP
};

// Message handler for the GStreamer loop
static gboolean bus_call(GstBus* bus,
                         GstMessage* msg,
                         gpointer data) {

    GMainLoop* loop = static_cast<GMainLoop*>(data);

    switch (GST_MESSAGE_TYPE(msg)) {

    case GST_MESSAGE_EOS:
        g_print("End of stream\n");
        g_main_loop_quit(loop);
        break;

    case GST_MESSAGE_ERROR: {
        gchar* debug;
        GError* error;

        gst_message_parse_error(msg, &error, &debug);
        g_free(debug);

        g_printerr("Error: %s\n", error->message);
        g_error_free(error);

        g_main_loop_quit(loop);
        break;
    }
    default:
        break;
    }

    return TRUE;
}

// Needed to safely end the recording and avoid corrupt output file
static gboolean stop_recording(gpointer data) {

    GstElement* pipeline = static_cast<GstElement*>(data);

    g_print("Time limit reached. Stopping recording...\n");
    gst_element_send_event(pipeline, gst_event_new_eos());
    return FALSE;
}

int webcamCaptureTest() {

    // GStreamer elements
    GstElement* pipeline = nullptr;
    GstElement* source = nullptr;
    GstElement* converter = nullptr;
    GstElement* encoder = nullptr;
    GstElement* muxer = nullptr;
    GstElement* sink = nullptr;

    // GStreamer loop components
    GMainLoop* loop = nullptr;
    GstBus* bus = nullptr;
    GstStateChangeReturn ret;
    guint bus_watch_id;

    // Initialize all elements
    //@TODO: Look into future deprecation of kvideosrc, still supported for now. 
    pipeline = gst_pipeline_new("pipeline");
    source = gst_element_factory_make("ksvideosrc", "source");
    encoder = gst_element_factory_make("x264enc", "encoder");
    muxer = gst_element_factory_make("mp4mux", "muxer");
    converter = gst_element_factory_make("videoconvert", "converter");
    sink = gst_element_factory_make("filesink", "sink");

    // Check for null elements
    if (!pipeline || !source || !encoder || !muxer || !converter || !sink) {

        // @TODO: Make print for each element to indicate their status
        g_printerr("Not all elements created. Exiting...\n");
        return -1;
    }

    // Set video source
    g_object_set(sink, "location", "output.mp4", NULL);
    g_print("Set video sink\n");

    // Create the main loop
    loop = g_main_loop_new(NULL, FALSE);

    //Get pipeline's bus (message handler)
    bus = gst_element_get_bus(pipeline);
    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);
    g_print("Setup bus\n");

    // Add all elements together
    gst_bin_add_many(GST_BIN(pipeline), source, converter, encoder, muxer, sink, NULL);

    // Link the elements, note that the order must match the command line format.
    if (gst_element_link_many(source, converter, encoder, muxer, sink, NULL) != TRUE) {
        g_printerr("Elements could not be linked. Exiting...\n");
        gst_object_unref(pipeline);
        return -1;
    }
    g_print("Link elements\n");

    // Set the pipeline state to playing
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state. Exiting...\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // @TODO: Add control for timed recording, if desired, currently 10s by default
    // @TODO: Add start and stop command for recording
    g_timeout_add_seconds(10, stop_recording, pipeline);

    g_print("Begin stream...\n");
    g_main_loop_run(loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);
    g_print("Deleting pipeline\n");

    return 0;
}

int rtpCaptureTest() {

    // GStreamer elements
    GstElement* pipeline = nullptr;

    // GStreamer loop components
    GMainLoop* loop = nullptr;
    GstBus* bus = nullptr;
    GstStateChangeReturn ret;
    guint bus_watch_id;

    // Create entire pipeline using parse_launch, this must match the sender application configuration.
    pipeline = gst_parse_launch(
        "udpsrc port=5000 caps=\"application/x-rtp, media=video, encoding-name=H264, payload=96\" ! "
        "rtph264depay ! "
        "h264parse ! "
        "mp4mux ! "
        "filesink location=output.mp4",
        NULL);

    if (!pipeline) {
        g_printerr("Failed to create pipeline\n");
        return -1;
    }

    // Create the main loop
    loop = g_main_loop_new(NULL, FALSE);

    // Get pipeline's bus (message handler)
    bus = gst_element_get_bus(pipeline);
    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state. Exiting...\n");
        gst_object_unref(pipeline);
        return -1;
    }

    g_timeout_add_seconds(10, stop_recording, pipeline);

    g_print("Begin capturing test stream...\n");
    g_main_loop_run(loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);
    g_print("Deleting pipeline\n");

    return 0;
}

int main(int argc, char* argv[]) {
    
    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Initialize test type
    Test test = Test::None;

    // Read input arguments, only accepting specific tests at the moment.
    // @TODO: Handle more command line arguments and automize recording source, devices, format etc.
    // @TODO: Add user configuration file.
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--webcam") {
            test = Test::Webcam;
        }
        else if (arg == "--rtp") {
            test = Test::RTP;
        }
    }

    switch (test) {
    case Test::Webcam:
        // 1. Webcam test
        // Requires a webcam to be plugged in (currently assuming only one camera which is the default)
        //
        // Configuration was based on the following command:
        // gst-launch-1.0 -e ksvideosrc ! videoconvert !  x264enc ! mp4mux ! filesink location=output.mp4
        webcamCaptureTest();
        break;
    case Test::RTP:
        // 2. RTP capture test on local machine
        // Requires GStreamer process running in separate terminal to simulate RTP stream using the following command:
        // 
        // gst-launch-1.0 -v videotestsrc is-live=true ! 
        // x264enc tune=zerolatency bitrate=512 speed-preset=ultrafast ! 
        // rtph264pay config-interval=1 pt=96 ! 
        // udpsink host=127.0.0.1 port=5000
        rtpCaptureTest();
        break;
    case Test::None:
        std::cout << "GStreamer.exe <TEST>\n--webcam : Capture webcam test\n--rtp : Capture rtp stream test" << std::endl;
        break;
    }

    return 0;
}