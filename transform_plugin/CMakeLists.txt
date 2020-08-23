cmake_minimum_required(VERSION 2.8)
project (webrtc_transform)
set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
set(CMAKE_C_FLAGS "-g -Wno-deprecated")

add_definitions(-DWEBRTC_TRANSFORM_PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
add_definitions(-DVERSION="${VERSION_STRING}")
add_definitions(-DPACKAGE="webrtc_transform")
add_definitions(-DPACKAGE_NAME="webrtc_transform")

find_package(PkgConfig)

pkg_check_modules(PC_GIO REQUIRED  "gio-2.0 >= 2.22")
pkg_check_modules(PC_GST REQUIRED  "gstreamer-1.0 >= 1.8.0")
pkg_check_modules(PC_GST_BASE REQUIRED "gstreamer-base-1.0 >= 1.8.0")
pkg_check_modules(PC_GST_APP REQUIRED "gstreamer-app-1.0 >= 1.8.0")
pkg_check_modules(PC_GST_RTP REQUIRED "gstreamer-rtp-1.0 >= 1.8.0")
pkg_check_modules(PC_GST_AUDIO REQUIRED  "gstreamer-audio-1.0 >= 1.8.0")
pkg_check_modules(PC_GST_VIDEO REQUIRED  "gstreamer-video-1.0 >= 1.8.0")
pkg_check_modules(PC_GST_PBUTILS REQUIRED  "gstreamer-pbutils-1.0 >= 1.8.0")
pkg_check_modules(PC_GST_TAG REQUIRED  "gstreamer-tag-1.0 >= 1.8.0")
pkg_check_modules(PC_GST_SOUP REQUIRED  "libsoup-2.4")
#gstreamer-webrtc-1.0 gstreamer-sdp-1.0 libsoup-2.4 json-glib-1.0
pkg_check_modules(PC_GST_WEBRTC REQUIRED  "gstreamer-webrtc-1.0")
pkg_check_modules(PC_GST_SDP    REQUIRED  "gstreamer-sdp-1.0")
pkg_check_modules(PC_JSON_GLIB  REQUIRED  "json-glib-1.0")
if (PC_GST_FOUND)
  set(GSTREAMER_LIBRARIES
    ${PC_GIO_LIBRARIES}
    ${PC_GST_LIBRARIES}
    ${PC_GST_BASE_LIBRARIES}
    ${PC_GST_APP_LIBRARIES}
    ${PC_GST_RTP_LIBRARIES}
    ${PC_GST_AUDIO_LIBRARIES}
    ${PC_GST_VIDEO_LIBRARIES}
    ${PC_GST_TAG_LIBRARIES}
    ${PC_GST_SOUP_LIBRARIES}
    ${PC_GST_WEBRTC_LIBRARIES}
    ${PC_GST_SDP_LIBRARIES}
    ${PC_JSON_GLIB_LIBRARIES}
    )

  set(GSTREAMER_INCLUDE_DIRS 
	  ${PC_GST_INCLUDE_DIRS} 
	  ${PC_GST_SOUP_INCLUDE_DIRS}
	  ${PC_GST_WEBRTC_INCLUDE_DIRS}
	  ${PC_GST_SDP_INCLUDE_DIRS}
	  ${PC_JSON_GLIB_INCLUDE_DIRS}
	  )
  message(STATUS "GStreamer found ${PC_GST_VERSION}")
endif ()

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories("${PROJECT_SOURCE_DIR}/src")

add_subdirectory(src)
add_subdirectory(test)

