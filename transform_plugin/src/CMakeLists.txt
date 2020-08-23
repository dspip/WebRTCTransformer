set(LIBIDRINSERT_SOURCES
   gstidrinsert.c
   )
set(LIBC264DEPAY_SOURCES
   gstc264depay.c
   )
set(LIBMPEG4FILTER_SOURCES
   gstmpeg4filter.c
   )
set(LIBTIMESTAMP_SOURCES
   gsttimestamp.c
   )

set (STREAMSEND_SOURCES
	webrtc_send.c
	webrtc.c
    )

set (STREAMRECV_SOURCES
	webrtc_recv.c
	webrtc.c
    )

set(LIBRTPEXTENTION_SOURCES
   gstrtpextention.c
   )

set(LIBREMOVESEI_SOURCES
   gstremovesei.c
   )

include_directories(${GSTREAMER_INCLUDE_DIRS} )
add_library(idrinsert SHARED ${LIBIDRINSERT_SOURCES})
target_link_libraries(idrinsert ${GSTREAMER_LIBRARIES})
install(TARGETS idrinsert LIBRARY DESTINATION "/usr/lib/x86_64-linux-gnu/gstreamer-1.0/")

add_library(removesei SHARED ${LIBREMOVESEI_SOURCES})
target_link_libraries(removesei ${GSTREAMER_LIBRARIES})
install(TARGETS removesei LIBRARY DESTINATION "/usr/lib/x86_64-linux-gnu/gstreamer-1.0/")


add_library(rtpextention SHARED ${LIBRTPEXTENTION_SOURCES})
target_link_libraries(rtpextention ${GSTREAMER_LIBRARIES}) 
install(TARGETS rtpextention  LIBRARY DESTINATION "/usr/lib/x86_64-linux-gnu/gstreamer-1.0/")

add_library(c264depay SHARED ${LIBC264DEPAY_SOURCES})
target_link_libraries(c264depay ${GSTREAMER_LIBRARIES})
install(TARGETS c264depay LIBRARY DESTINATION "/usr/lib/x86_64-linux-gnu/gstreamer-1.0/")


add_library(mpeg4filter SHARED ${LIBMPEG4FILTER_SOURCES})
target_link_libraries(mpeg4filter ${GSTREAMER_LIBRARIES})
install(TARGETS mpeg4filter LIBRARY DESTINATION "/usr/lib/x86_64-linux-gnu/gstreamer-1.0/")


add_library(timestamp SHARED ${LIBTIMESTAMP_SOURCES})
target_link_libraries(timestamp ${GSTREAMER_LIBRARIES})
install(TARGETS timestamp LIBRARY DESTINATION "/usr/lib/x86_64-linux-gnu/gstreamer-1.0/")


add_executable(stream_send ${STREAMSEND_SOURCES})
target_link_libraries(stream_send ${GSTREAMER_LIBRARIES})
#install(TARGETS stream_send EXECUTABLE DESTINATION "lib")

add_executable(stream_recv ${STREAMRECV_SOURCES})
target_link_libraries(stream_recv ${GSTREAMER_LIBRARIES})
#install(TARGETS stream_recv EXECUTABLE DESTINATION "lib")
