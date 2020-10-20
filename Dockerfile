FROM nvcr.io/nvidia/deepstream:5.0.1-20.09-base
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update
RUN apt-get install -y gstreamer1.0-tools gstreamer1.0-nice gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-plugins-good libgstreamer1.0-dev libglib2.0-dev libgstreamer-plugins-bad1.0-dev libsoup2.4-dev libjson-glib-dev
RUN apt-get install -y cmake git ninja-build zip ubuntu-restricted-extras ffmpeg
RUN apt-get install 
RUN apt-get autoremove -y

# RUN mkdir /opts && cd /opts \
# && git clone https://github.com/Microsoft/vcpkg.git && cd ./vcpkg \
# && ./bootstrap-vcpkg.sh -useSystemBinaries \
# && ./vcpkg integrate install

# RUN opts/vcpkg/vcpkg install boost 
# RUN ./vcpkg install nlohmann-json

RUN mkdir -p /app/WebRTCTransformer
WORKDIR /app/WebRTCTransformer
COPY . .

RUN cd transform_plugin && mkdir build && cd build \ 
    && cmake .. \
    && cmake --build . --config Release --target all -- -j 6  \
    && make install

ENTRYPOINT [ "./transform_plugin/build/src/stream_send" ]