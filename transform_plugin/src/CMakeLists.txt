set(LIBIDRINSERT_SOURCES
   gstidrinsert.c
   )
include_directories(${GSTREAMER_INCLUDE_DIRS} )
add_library(idrinsert SHARED ${LIBIDRINSERT_SOURCES})
target_link_libraries(idrinsert ${GSTREAMER_LIBRARIES})
install(TARGETS idrinsert LIBRARY DESTINATION "lib")
