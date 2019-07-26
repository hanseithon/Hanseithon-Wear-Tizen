#ifndef __PUSH_RECEIVER_H__
#define __PUSH_RECEIVER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tizen.h>
#include <bundle.h>
#include <dlog.h>
#include <push-service.h>

#if !defined(PACKAGE)
#define PACKAGE "org.example.pushreceiver"
#endif

#define RETURN_IF_FAIL(check_condition) \
		do {	\
			if (!(check_condition)) {	\
				dlog_print(DLOG_WARN, LOG_TAG, "Return by [" #check_condition "] is failed");	\
				return;	\
			}	\
		} while (0)

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "공휴 - 휴를 공유하다"


void deliver_message(bundle *msg_bundle);

void handle_push_state(const char *state, void *user_data);

void handle_push_message(const char *data, const char *msg, long long int time_stamp, void *user_data);

void on_state_registered(push_service_connection_h push_conn, void *user_data);

void on_state_unregistered(void *user_data);

void on_state_error(const char *err, void *user_data);

void parse_notification_data(push_service_notification_h noti, void *user_data);

#endif /* __PUSH_RECEIVER_H__ */
