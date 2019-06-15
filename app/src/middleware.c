#include <misc.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <middleware.h>
#include <zmq.h>
#include <jansson.h>

#define THIS_FILE "middleware.c"

/**************************************************************************
 * GLOBAL VARIABLES
 **************************************************************************/
void *context;
void *publisher;
void *subscriber;
int webdbsend_id = 3;

static int startMiddlewarePublish() {

  publisher = zmq_socket(context, ZMQ_PUB);
  if (zmq_bind (publisher, ZMQ_PUBLISHER_ADDR) == 0) {
    return SUCCESS;
  }

	return ERROR;
}

BOOL openMiddleware() {

	LOG("openMiddleware");

	context = zmq_ctx_new();

	if (startMiddlewarePublish() == ERROR) {
		LOG_ERROR("Erro ao inicilizar Middleware Publisher");
		return ERROR;
	}

	return SUCCESS;
}

int stopZeroMQ() {
  zmq_close(publisher);
  zmq_close(subscriber);
  zmq_ctx_destroy(context);
}

//  Convert C string to 0MQ string and send to socket
static int s_send (void *socket, char *string) {
  int size = zmq_send (socket, string, strlen (string), 0);
  return size;
}

//  Sends string as 0MQ string, as multipart non-terminal
static int s_sendmore (void *socket, char *string) {
  int size = zmq_send (socket, string, strlen (string), ZMQ_SNDMORE);
  return size;
}

void sendMiddlewareMessage(char *pchTales) {

	char *pchData;
  json_t *pResult;
  json_t *j_data;

  pResult = json_array(); 
  if (pResult == NULL) {
    return;
  }

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(pResult); 
    return;
  } 
  
  json_object_set_new(j_data, PARAM_APP_ID, json_integer(webdbsend_id));
  json_object_set_new(j_data, PARAM_TABLES, json_string(pchTales));

  json_array_append_new(pResult, j_data);  

  pchData = json_dumps(pResult, JSON_INDENT(2));
  json_decref(pResult);   

	s_sendmore(publisher, ENVELOPE_ZMQ_DATABASE);
  s_send(publisher, pchData);
}
