# GStreamer Video Recorder

GStreamer Video Recorder is a lightweight application for capturing video using the GStreamer multimedia framework. 

## Requirements
- GStreamer 1.x installed
- Development libraries (if building from source)

## Installation

### Building with Visual Studio
```sh
1. Add GSTREAMER_1_0_ROOT_MSVC_X86_64 to your system environment variables
2. Add GStreamer bin directory to the PATH
3. Start x64 Native Tools Command Prompt and navigate to the repository

mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE={Release/Debug}
cmake --build . --config {Release/Debug}
```

### Running the Application
```sh
{Release/Debug}/GStreamerApp.exe
```
