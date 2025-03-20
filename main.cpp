#include <gst/gst.h>
#include <iostream>

int main(int argc, char *argv[]) {
    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Print GStreamer version
    guint major, minor, micro, nano;
    gst_version(&major, &minor, &micro, &nano);
    std::cout << "GStreamer version: " << major << "." << minor << "." << micro << std::endl;

    // Create a simple pipeline: "videotestsrc ! autovideosink"
    GstElement *pipeline = gst_parse_launch("videotestsrc ! autovideosink", nullptr);
    if (!pipeline) {
        std::cerr << "Failed to create pipeline." << std::endl;
        return -1;
    }

    // Start playing
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to start playback." << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    std::cout << "Playing test video... Press Ctrl+C to stop." << std::endl;

    // Run the main event loop
    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(loop);

    // Clean up
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);

    return 0;
}
