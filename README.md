# WebRTCTransformer
WebRTCTransformer

#Install dependency
To install dependency of this project run 
sudo apt-get install -y gstreamer1.0-tools gstreamer1.0-nice gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-plugins-good libgstreamer1.0-dev git libglib2.0-dev libgstreamer-plugins-bad1.0-dev libsoup2.4-dev libjson-glib-dev


#Compile and Run idrinsert plugin

cd  transform_plugin
mkdir build
cd build
cmake ..
make 
gst-launch-1.0 --gst-plugin-path=src filesrc location=../nature_704x576_25Hz_1500kbits.h264  !  h264parse ! "video/x-h264, stream-format=byte-stream" ! idrinsert ! filesink location=1.264


#Running WebRTC application

cd transform_plugin
mkdir build
cd build
cmake ..
make

now there will be application stream_send and stream_recv in src directory.

To run webrtc stream send
run export GST_PLUGIN_PATH=src
./src/stream_send <H264 file path> <1/0 to add or remove filter>
e.g. ./src/stream_send /H264_6016.h264  1

WebRTC page link: http://127.0.0.1:57778/

Now write http://127.0.0.1:57778/ firefox browser, It will start playing video stream start by webrtc stream.

To Receive throgh webrtc gstreamer application

Run 
./src/stream_receive ws://127.0.0.1:57779/ws

This will display the video streamed by stream_send application


