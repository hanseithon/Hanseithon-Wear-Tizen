#include <app_preference.h>
#include <json-glib/json-glib.h>
#include <openssl/sha.h>

#include "pushreceiver.h"

#define PUSH_HASH_KEY	 "existing_push_reg_id"
#define BUNDLE_KEY_MSG "msg"
#define BUNDLE_KEY_TIME "time"


/*
 * @brief	Enumerations of custom protocol type value
 * @details	In this sample, app server set to protocol type on push appData field as json format
 * Likes this,
 * appData : "{\"type\": 1, ...<other custom fields>... }
 */
/*
typedef enum {
	CUSTOM_PROTOCOL_TYPE_MESSAGE	= 0,	// < normal message (we will launch UI application and forward data)
	CUSTOM_PROTOCOL_TYPE_FILE		= 1,		// < background protocol message (we will run background job without launching UI application)
} custom_protocol_type_e;
*/


/*
 * @brief	Message parser for Push appData field as application protocol (You can skip or change this logic)
 */
/* static int __custom_push_app_data_parser(const char *raw_data, int *type, char **message); */


/*
 * save registration id to file
 */
static void
save_reg_id_to_file(char *text)
{
	dlog_print(DLOG_DEBUG, LOG_TAG, "save_reg_id_to_file()");

	char path[512] = {0,};
	char *appid;
	app_get_id(&appid);

	RETURN_IF_FAIL(appid);

	strcat(path, "/opt/usr/media/");
	strcat(path, appid);
	strcat(path, "_reg_id.txt");
	dlog_print(DLOG_DEBUG, LOG_TAG, "path: [%s]", path);

	FILE *f;
	f = fopen(path, "w");
	if (f == NULL) {
		dlog_print(DLOG_DEBUG, LOG_TAG, "fopen() is failed");
		return;
	}

	fprintf(f, "%s", text);
	fclose(f);
}


/*
 * send registration id to application server
 */
static int
send_reg_id(const char *reg_id)
{
	/* TODO : This log print should be removed after test */
	dlog_print(DLOG_DEBUG, LOG_TAG, "[Do Not Print This] registration_id for debug : [%s]", reg_id);

	/* Your implementation here to send reg_id to your application server */

	save_reg_id_to_file((char *)reg_id);

	/* return 0 on success */
	return 0;
}


/*
 * send registration id if necessary
 */
static void
send_reg_id_if_necessary(const char *reg_id)
{
	char *hash_value = NULL;
	char *stored_hash_value = NULL;
	unsigned char md[SHA_DIGEST_LENGTH];

	/* Generate a hash string from reg_id */
	hash_value = (char *)SHA1((unsigned char *)reg_id, sizeof(reg_id), md);
	if (!hash_value) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Hash value is NULL");
		return;
	}

	dlog_print(DLOG_DEBUG, LOG_TAG, "Get hash [%s][%p]", md, md);
	/* Get the stored hash string */
	int ret = preference_get_string(PUSH_HASH_KEY, &stored_hash_value);

	/*
	 * If there is no hash string stored before or
	 * if the stored hash string is different from the new one,
	 * send reg_id to the server
	 */

	if (ret != PREFERENCE_ERROR_NONE || strcmp(stored_hash_value, hash_value) != 0) {
		dlog_print(DLOG_DEBUG, LOG_TAG, "Sending a new reg_id...");
		/* Your implementation here to send reg_id to your application server */
		ret = send_reg_id(reg_id);

		/* If reg_id is successfully sent, store the new hash value */
		if (!ret) {
			dlog_print(DLOG_DEBUG, LOG_TAG, "Save the hash key");
			ret  = preference_set_string(PUSH_HASH_KEY, hash_value);
			if (ret != PREFERENCE_ERROR_NONE)
				dlog_print(DLOG_ERROR, LOG_TAG, "Fail to save hash_value");
		}
	}

	if (stored_hash_value)
		free(stored_hash_value);

	/* DO NOT free hash_value because it is pointing char array md */

	return;
}


/*
 * parse push message
 */
static char *
get_value_from_message(char *text, char *key)
{
	/* Parsing arrival message or time */
	/* dlog_print(DLOG_DEBUG, LOG_TAG, "message : [%s] / key : [%s]", text, key); */
	/* dlog_print(DLOG_DEBUG, LOG_TAG, "time : [%s] / key : [%s]", text, key); */
	char *token = strtok(text, "=&");
	if (!strncmp(token, key, sizeof(*key))) {
		token = strtok(NULL, "=&");
	} else {
		while (token != strtok(NULL, "=&")) {
			token = strtok(NULL, "=&");
			if (!strcmp(token, key)) {
				token = strtok(NULL, "=&");
				break;
			}
		}
	}

	return token;
}


/*
 * function for handling push state
 */
void
handle_push_state(const char *state, void *user_data)
{
	dlog_print(DLOG_DEBUG, LOG_TAG, "State : [%s]", state);

	/* Add your code here */
}


/*
 * function for handling push message
 */
void
handle_push_message(const char *data, const char *msg, long long int time_stamp, void *user_data)
{
	/* Custom protocol parsing logic for sample */
	dlog_print(DLOG_DEBUG, LOG_TAG, "[Noti Data] push data : [%s]", data);
	dlog_print(DLOG_DEBUG, LOG_TAG, "[Noti Data] message : [%s]", msg);
	dlog_print(DLOG_DEBUG, LOG_TAG, "[Noti Data] time stamp : [%lld]", time_stamp);

	/* Get action value and alertMessage value */
	char *action = get_value_from_message(strdup(msg), "action");
	char *alert_message = get_value_from_message(strdup(msg), "alertMessage");
	dlog_print(DLOG_DEBUG, LOG_TAG, "------------- action: [%s] ---------------", action);
	dlog_print(DLOG_DEBUG, LOG_TAG, "------------ alert_message: [%s] ------------", alert_message);

	RETURN_IF_FAIL(action);
	RETURN_IF_FAIL(alert_message);

	/* Create arrival time & message */
	char buf_msg[512];
	sprintf(buf_msg, "");
	strcat(buf_msg, action);
	strcat(buf_msg, " / ");
	strcat(buf_msg, alert_message);

	time_t timer = time(NULL);
	struct tm *t;
	t = localtime(&timer);
	char buf_time[512];
	sprintf(buf_time, "%d%02d%02d %02d:%02d:%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

	dlog_print(DLOG_DEBUG, LOG_TAG, "display_msg: [%s]", buf_msg);
	dlog_print(DLOG_DEBUG, LOG_TAG, "display_time: [%s]", buf_time);

	bundle *msg_bundle = bundle_create();
	if (msg_bundle) {
		/* Payload push message to bundle object */
		bundle_add_str(msg_bundle, BUNDLE_KEY_MSG, strdup(buf_msg));
		dlog_print(DLOG_DEBUG, LOG_TAG, "message [%s], key [%s]", buf_msg, BUNDLE_KEY_MSG);
		bundle_add_str(msg_bundle, BUNDLE_KEY_TIME, strdup(buf_time));
		dlog_print(DLOG_DEBUG, LOG_TAG, "time [%s], key [%s]", buf_time, BUNDLE_KEY_TIME);
		deliver_message(msg_bundle);
	}

	bundle_free(msg_bundle);
	/* Custom protocol logic End */
}


/*
 * registration state callback for registered
 */
void
on_state_registered(push_service_connection_h push_conn, void *user_data)
{
	RETURN_IF_FAIL(push_conn);

	/* Request unread notifications to the push service */
	/* notification_message_cb() will be called if there is any unread notification */
	int ret = push_service_request_unread_notification(push_conn);
	if (ret != PUSH_SERVICE_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "ERROR : push_service_request_unread_notification() is failed [%d : %s]", ret, get_error_message(ret));
		return;
	}

	/* Get the registration id */
	char *reg_id = NULL;
	ret = push_service_get_registration_id(push_conn, &reg_id);
	if (ret != PUSH_SERVICE_ERROR_NONE || !reg_id) {
		dlog_print(DLOG_ERROR, LOG_TAG, "ERROR : push_service_get_registration_id() is failed [%d : %s]", ret, get_error_message(ret));
		return;
	}

	/* Send reg_id to your application server if it is necessary */
	send_reg_id_if_necessary(reg_id);

	free(reg_id);
}


/*
 * registration state callback for unregistered
 */
void
on_state_unregistered(void *user_data)
{
	dlog_print(DLOG_DEBUG, LOG_TAG, "Unregistered callback started");

	/* Reset the previously-stored registration id as blank */
	int ret  = preference_set_string(PUSH_HASH_KEY, "");
	if (ret != PREFERENCE_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "ERROR : Fail to initialize hash_value [%d : %s]", ret, get_error_message(ret));

	/* Add your code here */
}


/*
 * registration state callback for treating error
 */
void
on_state_error(const char *err, void *user_data)
{
	dlog_print(DLOG_ERROR, LOG_TAG, "ERROR : state [%s]", err);

	/* Add your code here */
}


/*
 * parse notification data
 */
void
parse_notification_data(push_service_notification_h noti, void *user_data)
{
	char *data = NULL;				/* App data loaded on the notification */
	char *msg = NULL;				/* Notification message */
	long long int time_stamp = 0LL;	/* Time when the notification message is generated */

	if (!noti) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Notification is Null");
		return;
	}

	/* Retrieve app data from notification handle */
	int ret = push_service_get_notification_data(noti, &data);
	if (ret != PUSH_SERVICE_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "ERROR : Fail to get notification data [%d : %s]", ret, get_error_message(ret));

	/* Retrieve notification message from notification handle */
	ret = push_service_get_notification_message(noti, &msg);
	if (ret != PUSH_SERVICE_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "ERROR : Fail to get notification message [%d : %s]", ret, get_error_message(ret));

	/* Retrieve the time from notification handle */
	ret = push_service_get_notification_time(noti, &time_stamp);
	if (ret != PUSH_SERVICE_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "ERROR : Fail to get notification time [%d : %s]", ret, get_error_message(ret));

	/* Your implementation here to use type, data, msg, and time_stamp */
	handle_push_message(data, msg, time_stamp, user_data);

	/* Do not free notification handle in the callback function */

	/* Free data and message */
	if (data)
		free(data);
	if (msg)
		free(msg);
}


/*
 * @brief	custom appData parser
 * @details	It depend on message format from app server
 * In this sample, we suppose the message is json format as below
 *
 * {
 *		"type" : <number>,
 *		"message" : "<string>"
 * }
 *
 * +----------------+---------------+-------------------------------+
 * | type			| message		| action						|
 * +----------------+---------------+-------------------------------+
 * | 0 (message)	| message text	| Launch application			|
 * | 1 (file)		| download URL	| File download on background	|
 * +----------------+---------------+-------------------------------+
 *
 * if json 'type' field data is 0, this app will launch UI app
 * if json 'type' field data is 1, this app will download file
 *
 * Sample server message data :
 * {
 *		"regID" : "<register ID>",
 *		"message" : "action=LAUNCH",
 *		"appData" : "{\"type\":0, \"message\":\"hello push\"}",
 *		"timeStamp" : 1234
 * }
 *
 * @param[in] raw_message	message raw data string
 * @param[out] type			message type (you can define to other meaning)
 * @param[out] message		text message  (you can define to other meaning)
 * @remarks	you must free about \a message
 */
/*
static int __custom_push_app_data_parser(const char *raw_data, int *type, char **message)
{
	int ret = -1;

	if (!raw_data)
		return ret;

#if !GLIB_CHECK_VERSION(2, 35, 0)
	g_type_init();
#endif

	JsonParser *parser = json_parser_new();
	if (!parser)
		return ret;

	// Parser load to Json object from raw json string
	json_parser_load_from_data(parser, raw_data, strlen(raw_data), NULL);

	JsonNode *node = json_parser_get_root(parser);

	if (node) {
		// Check json validation
		if (json_node_get_node_type(node) == JSON_NODE_OBJECT) {
			JsonObject *jobj = NULL;
			jobj = json_node_get_object(node);
			if (jobj) {
				// Gets 'type' element as custom protocol
				JsonNode *type_node = json_object_get_member(jobj, "type");
				// Gets 'message' element as custom protocol
				JsonNode *msg_node = json_object_get_member(jobj, "message");

				int _msg_type = 0;
				char *_text_msg = NULL;
				if (type_node && msg_node) {
					_msg_type = (int)json_node_get_int(type_node);
					dlog_print(DLOG_DEBUG, LOG_TAG, "custom msg type : %d", _msg_type);

					_text_msg = json_node_dup_string(msg_node);
					dlog_print(DLOG_DEBUG, LOG_TAG, "custom text msg : %s", _text_msg);

					*type = _msg_type;
					*message = _text_msg;
					ret = 0;
				}
			}
		}
	}

	g_object_unref(parser);

	return 0;
}
*/
