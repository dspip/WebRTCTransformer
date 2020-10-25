# needs stuff from: 

[https://github.com/NVIDIA-AI-IOT/deepstream_tlt_apps](https://github.com/NVIDIA-AI-IOT/deepstream_tlt_apps)

* gonna need to fix imports and pipeline's staticly set pgie config location

## send file:
      gst-launch-1.0 multifilesrc loop=true location=/home/yossi/share/sample_720p.h264 ! h264parse ! rtph264pay ! "application/x-rtp,payload=(int)96,clock-rate=(int)90000" ! udpsink host=127.0.0.1 port=5000 -v

## play:

      ./src/stream_send 5 127.0.0.1 5000