
set (RTPGEN_SOURCES    
      rtp_generator.c
)

include_directories(${GSTREAMER_INCLUDE_DIRS} )
add_executable(rtp_generator ${RTPGEN_SOURCES})
target_link_libraries(rtp_generator ${GSTREAMER_LIBRARIES})
