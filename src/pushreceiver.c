#include <app_control.h>
#include <efl_extension.h>
#include <glib.h>
#include <system_settings.h>

#include "pushreceiver.h"

/* Below PUSH_APP_ID should be changed with your own Push App ID */
#define PUSH_APP_ID "51642f356c585db3"
#define BUNDLE_KEY_MSG "msg"
#define BUNDLE_KEY_TIME "time"
#define PUSH_MESSAGE_SIZE 4096

/* app_data structure for drawing UI of Push Receiver */
typedef struct app_data {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *layout;
	Evas_Object *nf;
	/* Object for circular UI */
	Eext_Circle_Surface *circle_surface;
} app_data_s;

/* Declare global app_data object */
app_data_s *g_ad = NULL;

/* item_data structure for generating genlist */
typedef struct item_data {
	Elm_Object_Item *item;
	int index;
} item_data_s;

/* GList for push messages */
static GList *message_list = NULL;
static GList *time_list = NULL;

/* Variable for counting the number of push message */
static int cnt_push = 0;

/* Push service connection handle */
push_service_connection_h push_conn = NULL;

/* Declare create_list_view() */
static void create_list_view(void);


/*
 * save_message() for storing push messages
 */
static void
save_message(const char *message)
{
	/* In this sample, just store to GList */
	char *msg = NULL;

	if (message) msg = strdup(message);
	if (!msg) return;

	/* This list will be appended at top of the list */
	message_list = g_list_prepend(message_list, (gpointer)msg);
}


/*
 * save_time() for storing time with its push messages
 */
static void
save_time(const char *time)
{
	/* In this sample, just store to GList */
	char *msg = NULL;

	if (time) msg = strdup(time);
	if (!msg) return;

	/* This list will be appended at top of the list */
	time_list = g_list_prepend(time_list, (gpointer)msg);
}


/*
 * display_message() for displaying messages on application's main view
 */
static void
display_message(const char *message)
{
	dlog_print(DLOG_INFO, LOG_TAG, "[Push Message] %s", message);

	/* Increase the number of push message for enabling clear button */
	cnt_push++;

	/* Re-draw the list for verifying the arrival of push messages */
	create_list_view();
}


/*
 * deliver_message() for receiving push messages from push_data_handler.c
 */
void
deliver_message(bundle *msg_bundle)
{
	dlog_print(DLOG_DEBUG, LOG_TAG, "deliver_message() operates saving and showing push messages");
	if (msg_bundle) {
		char *text = NULL;

		/* Store push message */
		int ret = bundle_get_str(msg_bundle, BUNDLE_KEY_MSG, &text);
		if (!ret && text) {
			dlog_print(DLOG_DEBUG, LOG_TAG, "Push on message port : [%s]", text);
			/* Save message if necessary */
			save_message(text);
		}

		/* Store verification time and display push message and time */
		ret = bundle_get_str(msg_bundle, BUNDLE_KEY_TIME, &text);
		if (!ret && text) {
			dlog_print(DLOG_DEBUG, LOG_TAG, "Push on time port : [%s]", text);
			/* Save time if necessary */
			save_time(text);
			/* UI display function not for changed registration state but arrived push notification */
			display_message(text);
		}
	}
}


/*
 * callback for naviframe
 */
static void
naviframe_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	/* Store app_data */
	app_data_s *ad = data;
	elm_win_lower(ad->win);
}


/*
 * callback for window
 */
static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}


/*
 * _gl_push_title_text_get() for getting title of Push Receiver
 */
static char *
_gl_push_title_text_get(void *data, Evas_Object *obj, const char *part)
{
	return strdup("공휴 - 휴를 공유하다");
}


/*
 * _gl_push_text_get() for getting GList text to list up
 */
static char *
_gl_push_text_get(void *data, Evas_Object *obj, const char *part)
{
	char msg_buf[PUSH_MESSAGE_SIZE];
	char time_buf[PUSH_MESSAGE_SIZE];
	item_data_s *id = (item_data_s *)data;
	int index = id->index;
	char *msg = NULL;

	if (!strcmp("elm.text", part)) {
		msg = (char *) g_list_nth_data(message_list, index);
		snprintf(msg_buf, PUSH_MESSAGE_SIZE - 1, "%s", msg);
		dlog_print(DLOG_ERROR, LOG_TAG, "_gl_push_text_get(), message : [%s]", msg_buf);
		return strdup(msg_buf);
	}

	msg = (char *) g_list_nth_data(time_list, index);
	snprintf(time_buf, PUSH_MESSAGE_SIZE - 1, "%s", msg);
	dlog_print(DLOG_ERROR, LOG_TAG, "_gl_push_text_get(), time : [%s]", time_buf);
	return strdup(time_buf);
}


/*
 * clear_message_list() for removing all the list of push messages
 */
static void
clear_message_list(void)
{
	g_list_free_full(message_list, free);
	g_list_free_full(time_list, free);
	message_list = NULL;
	time_list = NULL;
}


/*
 * callback for operation when clear button is pressed
 */
static void
on_clear_btn_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
	/* Decrease the number of push message for disabling clear button */
	cnt_push = 0;

	/* Drawing list view after clearing push message list */
	clear_message_list();
	create_list_view();
}


/*
 * create_list_view for displaying the contents of push messages
 */
static void
create_list_view(void)
{
	Evas_Object *clear_button, *genlist = NULL, *circle_genlist;

	/* Elm_Genlist_Item_Class for title, list, padding */
	Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
	Elm_Genlist_Item_Class *ttc = elm_genlist_item_class_new();
	Elm_Genlist_Item_Class *ptc = elm_genlist_item_class_new();

	/* item_data for genlist */
	item_data_s *id;
	guint message_length = 0;

	/* app_data */
	app_data_s *ad = NULL;
	ad = (app_data_s *) g_ad;
	if (ad == NULL) return;

	genlist = elm_genlist_add(ad->nf);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	/* genlist for circular view */
	circle_genlist = eext_circle_object_genlist_add(genlist, ad->circle_surface);
	eext_circle_object_genlist_scroller_policy_set(circle_genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	eext_rotary_object_event_activated_set(circle_genlist, EINA_TRUE);

	ttc->item_style = "title";
	ttc->func.text_get = _gl_push_title_text_get;

	itc->item_style = "2text";
	itc->func.text_get = _gl_push_text_get;

	ptc->item_style = "padding";

	/* Append genlist item for title */
	elm_genlist_item_append(genlist, ttc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);

	/* Make list as much as the number of lines */
	message_length = g_list_length(message_list);
	int i = 0;
	for (i = 0; i < message_length; i++) {
		id = calloc(sizeof(item_data_s), 1);
		id->index = i;
		/* Append genlist item for list */
		id->item = elm_genlist_item_append(genlist, itc, id, NULL, ELM_GENLIST_ITEM_NONE, NULL, ad);
	}

	/* Append genlist item for padding */
	elm_genlist_item_append(genlist, ptc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);

	elm_genlist_item_class_free(itc);
	elm_genlist_item_class_free(ttc);
	elm_genlist_item_class_free(ptc);

	/* Add clear button for removing all the push messages on the list */
	elm_layout_theme_set(ad->layout, "layout", "bottom_button", "default");
	clear_button = elm_button_add(ad->nf);
	elm_object_style_set(clear_button, "bottom");
	elm_object_text_set(clear_button, "Clear");
	elm_object_part_content_set(ad->layout, "elm.swallow.button", clear_button);
	evas_object_smart_callback_add(clear_button, "clicked", on_clear_btn_selected_cb, NULL);

	/*
	 * "Clear" button will be located on bottom of the view
	 * The button is enable when there is a push message
	 */
	if (cnt_push > 0)
		elm_object_disabled_set(clear_button, EINA_FALSE);
	else
		elm_object_disabled_set(clear_button, EINA_TRUE);
	evas_object_show(clear_button);

	elm_naviframe_item_push(ad->nf, "공휴 - 휴를 공유하다.", NULL, NULL, genlist, "empty");
}


static void
create_base_gui(app_data_s *ad)
{
	/* Window */
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_conformant_set(ad->win, EINA_TRUE);
	elm_win_autodel_set(ad->win, EINA_TRUE);

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 4);
	}

	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);

	/* Conformant */
	ad->conform = elm_conformant_add(ad->win);
	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_OPAQUE);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	/* Eext Circle Surface Creation */
	ad->circle_surface = eext_circle_surface_conformant_add(ad->conform);

	/* Base Layout */
	ad->layout = elm_layout_add(ad->conform);
	evas_object_size_hint_weight_set(ad->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_layout_theme_set(ad->layout, "layout", "application", "default");
	evas_object_show(ad->layout);
	elm_object_content_set(ad->conform, ad->layout);

	/* Naviframe */
	ad->nf = elm_naviframe_add(ad->layout);
	elm_object_part_content_set(ad->layout, "elm.swallow.content", ad->nf);
	elm_naviframe_event_enabled_set(ad->nf, EINA_TRUE);
	eext_object_event_callback_add(ad->nf, EEXT_CALLBACK_BACK, naviframe_back_cb, ad);
	eext_object_event_callback_add(ad->nf, EEXT_CALLBACK_MORE, eext_naviframe_more_cb, NULL);

	/* Show Push Receiver main view*/
	create_list_view();

	/* Show window after base GUI is set up */
	evas_object_show(ad->win);
}


/*
 * registration result callback
 */
static void
registration_result_cb(push_service_result_e result, const char *msg, void *user_data)
{
	/* Also, registration_state_cb() will be invoked */
	if (result == PUSH_SERVICE_RESULT_SUCCESS)
		dlog_print(DLOG_DEBUG, LOG_TAG, "Registration request is approved.");
	else
		dlog_print(DLOG_ERROR, LOG_TAG, "Registration ERROR [%s]", msg);

	return;
}


/*
 * registration state callback
 */
static void
registration_state_cb(push_service_state_e state, const char *err, void *user_data)
{
	switch (state) {
	case PUSH_SERVICE_STATE_UNREGISTERED:
		dlog_print(DLOG_DEBUG, LOG_TAG, "Registration state is set as UNREGISTERED");
		handle_push_state("UNREGISTERED", user_data);
		on_state_unregistered(user_data);
		/* Register Push Receiver to push server */
		push_service_register(push_conn, registration_result_cb, user_data);
		break;

	case PUSH_SERVICE_STATE_REGISTERED:
		dlog_print(DLOG_DEBUG, LOG_TAG, "Registration state is set as REGISTERED");
		handle_push_state("REGISTERED", user_data);
		/*
		 * Important : you can get registration ID and send it to your app server if you need
		 * You must check if registration ID was changed
		 */
		on_state_registered(push_conn, user_data);
		break;

	case PUSH_SERVICE_STATE_ERROR:
		dlog_print(DLOG_DEBUG, LOG_TAG, "Registration state has an ERROR");
		handle_push_state("ERROR", user_data);
		on_state_error(err, user_data);
		break;

	default:
		dlog_print(DLOG_DEBUG, LOG_TAG, "Registration state is UNKNOWN [%d]", (int)state);
		break;
	}

	if (err)
		dlog_print(DLOG_DEBUG, LOG_TAG, "State error message [%s]", err);
}


/*
 * notification message callback
 */
static void
notification_message_cb(push_service_notification_h noti, void *user_data)
{
	dlog_print(DLOG_DEBUG, LOG_TAG, "Push Notification arrived");
	parse_notification_data(noti, user_data);
}


/*
 * function to connect Push Receiver with Push Service
 */
static bool
connect_push_service(void)
{
	dlog_print(DLOG_DEBUG, LOG_TAG, "Push Receiver is connected with push service");

	/* Connect to push service when the app is launched */
	int ret = push_service_connect(PUSH_APP_ID, registration_state_cb, notification_message_cb, NULL, &push_conn);
	if (ret != PUSH_SERVICE_ERROR_NONE) {
		dlog_print(DLOG_DEBUG, LOG_TAG, "ERROR : push_service_connect() is failed [%d : %s]", ret, get_error_message(ret));
		push_conn = NULL;
		return false;
	}

	return true;
}


/*
 * app create callback
 */
static bool
app_create(void *data)
{
	/* Hook to take necessary actions before main event loop starts
		Initialize UI resources and application's data
		If this function returns true, the main loop of application starts
		If this function returns false, the application is terminated */

	dlog_print(DLOG_DEBUG, LOG_TAG, "Push Receiver Create started");

	if (!push_conn) {
		bool ret = connect_push_service();
		/* If connect_push_service() is failed, Push Receiver will be terminated */
		if (!ret) {
			dlog_print(DLOG_DEBUG, LOG_TAG, "connect to push service is failed, application will be terminated");
			return false;
		}
	}

	/* app_data for drawing main UI interface */
	app_data_s *ad = data;
	/* Create base UI of application */
	create_base_gui(ad);

	return true;
}


/*
 * app terminate callback
 */
static void
app_terminate(void *data)
{
	dlog_print(DLOG_DEBUG, LOG_TAG, "Push Receiver Terminate");

	/* Release all push messages */
	clear_message_list();

	/* Disconnect Push Receiver from Push Service */
	if (push_conn)
		push_service_disconnect(push_conn);

	push_conn = NULL;
}


/*
 * app pause callback
 */
static void
app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible */
	dlog_print(DLOG_DEBUG, LOG_TAG, "Push Receiver Pause");

	/*
	 * Disconnect Push Receiver from Push Service
	 * for re-connection when the application is resume
	 */
	if (push_conn)
		push_service_disconnect(push_conn);

	push_conn = NULL;
}


/*
 * app resume callback
 */
static void
app_resume(void *data)
{
	/* Take necessary actions when application becomes visible */
	dlog_print(DLOG_DEBUG, LOG_TAG, "Push Receiver Resume");

	/* Retry connect with Push Service for proper operation */
	if (push_conn)
			push_service_disconnect(push_conn);

	push_conn = NULL;

	bool ret = connect_push_service();
	if (ret) {
		/*
		 * If connection with Push Service is succeeded,
		 * Push Receiver should request unread notification messages
		 * which are sent during disconnected state
		 */
		int result = push_service_request_unread_notification(push_conn);
		if (result != PUSH_SERVICE_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "ERROR : push_service_request_unread_notification() is failed [%d : %s]", result, get_error_message(result));
			return;
		}
	}
}


/*
 * app control callback
 */
static void
app_control(app_control_h app_control, void *data)
{
	dlog_print(DLOG_DEBUG, LOG_TAG, "Push Receiver's App Control callback is invoked");
}


/*
 * app low battery callback
 */
static void
ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/* APP_EVENT_LOW_BATTERY */
}


/*
 * app low memory callback
 */
static void
ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/* APP_EVENT_LOW_MEMORY */
}


/*
 * app orientation changed callback
 */
static void
ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/* APP_EVENT_DEVICE_ORIENTATION_CHANGED */
	return;
}


/*
 * app language changed callback
 */
static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/* APP_EVENT_LANGUAGE_CHANGED */
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}


/*
 * app region format changed callback
 */
static void
ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/* APP_EVENT_REGION_FORMAT_CHANGED */
}


/*
 * Push Receiver main
 */
int main(int argc, char *argv[])
{
	app_data_s ad = {0,};
	int ret = 0;

	/* Life cycle callback structure */
	ui_app_lifecycle_callback_s event_callback = {0,};
	/* Event handler */
	app_event_handler_h handlers[5] = {NULL, };

	/* Set event callback functions */
	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	/* Set low battery event */
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	/* Set low memory event */
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	/* Set orientation changed event */
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
	/* Set language changed event */
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	/* Set region format changed event */
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);
	ui_app_remove_event_handler(handlers[APP_EVENT_LOW_MEMORY]);

	g_ad = &ad;
	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "ERROR : app_main() is failed [%d : %s]", ret, get_error_message(ret));

	return ret;
}
