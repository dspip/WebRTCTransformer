#ifndef WEBRTC_H
#define WEBRTC_H
#include <stdio.h>
typedef void (*user_cb)(void *messege, void *user_data);
void initialize_ctx();
void *  start_webrtc_stream( char *host,
                char *port,
		int add_filter,
		user_cb userCb,
		void *user_data);
void delete_ctx (void *ctx);
int  webrtc_set_remote_sdp(void *ctx1, char *sdp_string);
int webrtc_set_remote_ice(void *ctx1, int mline_index, char *candidate_string);
void *receive_webrtc_stream (user_cb userCb,
		void *user_data);

#endif
