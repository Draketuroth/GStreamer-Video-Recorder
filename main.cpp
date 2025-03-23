#include <gst/gst.h>

// Command-line version
// gst-launch-1.0 -e ksvideosrc ! videoconvert !  x264enc ! mp4mux ! filesink location=output.mp4

// Implemented using GStreamer 1.26.0

// Message handler for the GStreamer loop
static gboolean
bus_call(GstBus* bus,
    GstMessage* msg,
    gpointer    data)
{
    GMainLoop* loop = (GMainLoop*)data;

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
gboolean stop_recording(gpointer data) {

    GstElement* pipeline = (GstElement*)data;

    g_print("Time limit reached. Stopping recording...\n");
    gst_element_send_event(pipeline, gst_event_new_eos());
    return FALSE;
}

int main(int argc, char* argv[]) {
    
    // @TODO: Handle command line arguments and automize recording source, devices, format etc.

    // GStreamer elements
    GstElement* pipeline;
    GstElement* source;
    GstElement* converter;
    GstElement* encoder;
    GstElement* muxer;
    GstElement* sink;

    // GStreamer loop components
    GMainLoop* loop;
    GstBus* bus;
    GstStateChangeReturn ret;
    guint bus_watch_id;

    gst_init(&argc, &argv);

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
    gst_object_unref(GST_OBJECT(pipeline));
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);
    g_print("Deleting pipeline\n");
}