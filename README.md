# Project Overview

## Setup Requirements:
### General Dependencies:
* ubuntu:
      
      sudo apt-get install cmake

### GStreamer Development Dependencies:
* ubuntu:

      sudo apt-get install -y gstreamer1.0-tools gstreamer1.0-nice gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-plugins-good libgstreamer1.0-dev libglib2.0-dev libgstreamer-plugins-bad1.0-dev libsoup2.4-dev libjson-glib-dev

### For WebRTC playback in Chrome:
+ open [chrome://flags](chrome://flags) url in Chrome, and change the following:
   + Disable [Anonymize local IPs exposed by WebRTC](https://support.senso.cloud/support/solutions/articles/17000097356-disabling-anonymize-local-ips-exposed-by-webrtc) - not disabling this might cause stun failure.
   + Enable Hardware Acceleration - 


_ _ _
## Compilation Guide:
CMake project is located inside `transform_plugin` folder.  
to compile the project follow these steps:
> cd ./transform_plugin  
cd  transform_plugin  
mkdir build  
cd build  
cmake ..  
cmake --build .  

These commands will compile the following addon plugins:  
* *c264depay*
* *idrinset* -  
   This is a gstreamer plugin which is used to add SEI header in incoming H264 stream.
* *mpeg4filter*
* *timestamp*

After compiling, inorder for GStreamer to be able to find the additional plugins, you can either copy the dll's (.so) files to GStreamer's plugin path: (`/usr/lib/x86_64-linux-gnu/gstreamer-1.0/` by default) manually or by executing :
> make install  

or you can define the `GST_PLUGIN_PATH` environment variable to a path containing the dlls, for example:  
>export GST_PLUGIN_PATH=/dev/WebRTCTransformer/transformer_plugin/build/src


In addition to the plugins, the following executables will be compiled:

* rtp_generator
* stream_recv
* stream_send
_ _ _


## RTP GENERATOR:
when we compile the code rtp_generator will be created in src/rtp_generator.

This application is used to send the rtp data to  webrtc transformer.

Usage :
`./rtp_generator <filename> <transcoding 1/0> <mpeg4-0 h264-1><address> <port> `

cases:
Note: First Two case will do  transcoding to generate specific codec output
1) Sending any file udp/rtp/h264
  ./rtp_generator file.mp4  1 0 127.0.0.1 5000
2)  Sending any file udp/rtp/mpeg4
  ./rtp_generator file.mp4  1 1 127.0.0.1 5000
3) Sending mp4/h264 without transcoding
  ./rtp_generator file.mp4  0 0 127.0.0.1 5000
4) Sending mp4/mpeg4 without transcoding
  ./rtp_generator file.mp4  0 1 127.0.0.1 5000

If we want to use  multicasting replace 127.0.0.1 with multicast address and port with multicast port.

_ _ _

## STREAM SEND:

This application is used to send H264/VP9 video stream to web browser through webrtc protocol.

To stream video to browser we need to run:

> `./src/stream_send <filter_option: 0-9> <src address> <udp port> <multicast interface on option 6>`  

filter_option : 
+ 0 - enbale filter               
+ 1 - disable filter     
+ 2 - remove filter
+ 3 - pass through H264 (No filter is added to graph)
+ 4 - pass through VP9
+ 5 - mpeg4/RTP
+ 6 - file to webrtc
+ 7 - multicast mpeg4 to webrtc
+ 8 - multicat mpeg4 fps config
+ 9 - multicast mpeg4 x264enc change fps

### Example:
command:
> ./src/stream_send 0 127.0.0.1(src address) 5555  

output:
> run http://127.0.0.1:57778 on browser to receive video stream  

explanation:  
The program will host a HTTP and WebSocket server on http://localhost:57778 and [ws://localhost:57778/ws](ws://127.0.0.1:57778/ws) respectively, inorder to establish a RTCPeerConnection with the media served on 127.0.0.1 port 5555.

Inorder to provide media for `stream_send` you can try the following commands:  
On a second terminal run :  

* if you are using `filter_option` value from 0 to 3, ie H264 media:
      
      gst-launch-1.0 videotestsrc ! videoconvert ! x264enc ! video/x-h264, profile=baseline ! rtph264pay config-interval=1 ! udpsink host=127.0.0.1 port=5555 -v

* if you are using `filter_option` value 4, ie VP9 media:  

      gst-launch-1.0 videotestsrc ! videoconvert ! vp9enc !  rtpvp9pay keyframe-max-dist=10 ! udpsink host=127.0.0.1 port=5555 -v
   


This is rtp data generator application, which is used to feed rtp packet to webrtc stream send application. stream_send application will receive these packets and forward them to browser for streaming purpose.


c) Now on browser run http://127.0.0.1:57778  to get video stream.

   Note:

   On most of the browser H264 baseline profile is supported. To enable H264 main and high profile we need to enable hardware accelaration on chrome browser. For that we need to install gpu driver on host machine.


   Also when we want to stream video to chrome browser run chrome://flags in it makesure to disable Anonymizedlocal ips exposed by webrtc.
_ _ _

## OLD DOCS REMNANTS

This project has two module
1) transform plugin : This is a gstreamer plugin which is used to add SEI header in incoming H264 stream.
2) webrtc interface: This module contains webrtc streaming application , which is used to send H264 video stream to browser through webrtc protocol.

Running the application:

1) Testing gstreamer transform plugin idrinsert:


gst-launch-1.0 filesrc location=../nature_704x576_25Hz_1500kbits.h264  !  h264parse ! "video/x-h264, stream-format=byte-stream" ! idrinsert enable=true ! filesink location=1.264


This will generate the modified H264 stream which has added SEI header in input H264 stream.


2) Running webrtc send streaming application  

d) Use javascript to receive video stream:

   Have put send.html file in it there is a java script code, which is used to receive the webrtc stream from the application. To get the webrtc stream from the specific host in playstream function pass host address and its websocket port number.

e.g. playStream(vidstream, "127.0.0.1", "57778", "ws", config, function (errmsg) 
     
3) Running stream receive 

   run 
   ./src/stream_recv ws://127.0.0.1:57779/ws


4) To test mpeg4 streaming 

1) Run gstreamer 
gst-launch-1.0 filesrc location=ducks_m4v.mp4 ! qtdemux ! queue ! rtpmp4vpay config-interval=1 ! udpsink host=127.0.0.1 port=5000 -v

This will generate below SDP
application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)MP4V-ES, profile-level-id=(string)1, config=(string)000001b001000001b58913000001000000012000c48d8800f50b04241463000001b24c61766335382e33352e313030, payload=(int)96, seqnum-offset=(uint)16107, timestamp-offset=(uint)1689274471, ssrc=(uint)1251088986, a-framerate=(string)30

2) Now run mpeg4 to webrtc pipeline with config 
./src/stream_send 5 127.0.0.1 5000
_ _ _ 

* To test the multicast mpeg4 stream:  
   > ./src/stream_send 6 239.3.0.1 6001 eno1

* To test file streaming:  
   > ./src/stream_send 6 ducks_m4v.mp4 

* Test multicast stream through command line:  
   > gst-launch-1.0  udpsrc address=239.3.0.1 port=6001 ! mpeg4filter ! mpeg4videoparse  ! timestamp ! avdec_mpeg4 !  videoconvert ! videorate ! video/x-raw, framerate=30/1 !  autovideosink -v