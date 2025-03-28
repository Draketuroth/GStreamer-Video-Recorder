cmake_minimum_required(VERSION 3.10)
project(GStreamerApp)

# Find GStreamer
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_BASE REQUIRED gstreamer-base-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED gstreamer-app-1.0)

# Include GStreamer headers
# Note: Header paths are generated incorrectly at the moment, setting manually for now.
include_directories(
  $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}/include/gstreamer-1.0/
  $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}/include/glib-2.0/
  $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}lib/glib-2.0/include
)
# Specify path to look for libraries
# Note: Library path is generated incorrectly at the moment, setting manually for now.
link_directories(
  $ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}/lib/
)

# Define the executable
add_executable(GStreamerApp main.cpp)

# Link GStreamer libraries
target_link_libraries(GStreamerApp ${GSTREAMER_LIBRARIES})