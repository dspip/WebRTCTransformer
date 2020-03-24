# WebRTCTransformer
WebRTCTransformer

#Compile and Run idrinsert plugin

cd  transform_plugin
mkdir build
cd build
cmake ..
make 
gst-launch-1.0 --gst-plugin-path=src filesrc location=../nature_704x576_25Hz_1500kbits.h264  !  h264parse ! "video/x-h264, stream-format=byte-stream" ! idrinsert ! filesink location=1.264
