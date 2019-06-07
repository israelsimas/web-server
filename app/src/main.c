/**************************************************************************
 *
 * web-server.c
 *
 *    Web server to Sirius
 *
 * Copyright 2019 Intelbras
 *
 **************************************************************************/

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ulfius.h>

#define PORT          8537
#define PREFIX        "/test"
#define PREFIXJSON    "/testjson"
#define PREFIXCOOKIE  "/testcookie"
#define STATIC_FOLDER "resources"

#define JSON1_URL "/db.cgi"
#define JSON2_URL "/db.cgi"
#define JSON3_URL "/status.cgi"

#define JSON1_CONTENT "[\n   {\n   \"rows\":[\n               {\n         \"SYSPhoneLanguage\":\"pt_BR\"\n         }\n      ]\n   }\n]"
#define JSON2_CONTENT "[\
   {\
   \"rows\":[\
               {\
         \"weekStart\":1,\
         \"SYSTimeNTPSecondAddress\":\"b.ntp.br\",\
         \"hourStop\":0,\
         \"SYSTimeManual\":0,\
         \"SYSTimeEnableNTP\":2,\
         \"weekDayStop\":1,\
         \"dayStart\":1,\
         \"SYSTimeNTPFirstAddress\":\"a.ntp.br\",\
         \"monthStart\":1,\
         \"offsetDaylightSaving\":1,\
         \"SYSTimeUpdateInterval\":10,\
         \"dayStop\":1,\
         \"hourStart\":0,\
         \"SYSTimeDayligthSaving\":0,\
         \"monthStop\":2,\
         \"weekStop\":1,\
         \"weekDayStart\":1,\
         \"SYSTimeDateFormat\":\"DD/MM/AA\",\
         \"SYSTimeTimeZone\":-181,\
         \"SYSTimeFormat\":\"HH:MM:ss\"\
         }\
      ]\
   }\
]"

#define JSON3_CONTENT "{\n\
\"account\":   {\n\
   \"user4\":\"0\",\n\
   \"user3\":\"0\",\n\
   \"user2\":\"0\",\n\
   \"user1\":\"200\"\n\
   },\n\
\"net\":   {\n\
   \"add_ipv6\":\"\",\n\
   \"add_ipv4\":\"10.1.39.66\",\n\
   \"netmask\":\"255.255.255.0\",\n\
   \"gateway_ipv6\":\"\",\n\
   \"dns2_ipv6\":\"\",\n\
   \"dns1_ipv6\":\"\",\n\
   \"type_ipv6\":\"\",\n\
   \"gateway_ipv4\":\"10.1.39.1\",\n\
   \"dns1_ipv4\":\"8.8.8.8\",\n\
   \"type_ipv4\":0,\n\
   \"mac\":\"00:1A:3F:01:09:19\",\n\
   \"dns2_ipv4\":\"\",\n\
   \"prot_mode\":0\n\
   },\n\
\"system\":   {\n\
   \"swMinor\":\"1\",\n\
   \"server_host\":\"10.1.39.66\",\n\
   \"tmp_op\":\"6674\",\n\
   \"time12h\":\"01:50:36\",\n\
   \"date\":\"01/01/2017\",\n\
   \"time24h\":\"01:50:36\",\n\
   \"swMajor\":\"0\",\n\
   \"user\":\"admin\",\n\
   \"host_name\":\"intelbras\",\n\
   \"freePartition\":1,\n\
   \"swPatch\":\"1\",\n\
   \"hwVersion\":\"21\",\n\
   \"tmp_ntp\":1483246236\n\
   },\n\
\"product\":\"tip1001\",\n\
\"branch\":\"D\"\n\
}"

/**
 * callback functions declaration
 */
int callback_get_test (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_get_empty_response (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_post_test (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_all_test_foo (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_get_cookietest (const struct _u_request * request, struct _u_response * response, void * user_data);

///////////////
int callback_json1_test(const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_json2_test(const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_json3_test(const struct _u_request * request, struct _u_response * response, void * user_data);
///////////////

// Callback function used to serve static files that are present in the static folder
int callback_static_file (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_default (const struct _u_request * request, struct _u_response * response, void * user_data);

/**
 * decode a u_map into a string
 */
char * print_map(const struct _u_map * map) {
  char * line, * to_return = NULL;
  const char **keys, * value;
  int len, i;
  if (map != NULL) {
    keys = u_map_enum_keys(map);
    for (i=0; keys[i] != NULL; i++) {
      value = u_map_get(map, keys[i]);
      len = snprintf(NULL, 0, "key is %s, value is %s", keys[i], value);
      line = o_malloc((len+1)*sizeof(char));
      snprintf(line, (len+1), "key is %s, value is %s", keys[i], value);
      if (to_return != NULL) {
        len = o_strlen(to_return) + o_strlen(line) + 1;
        to_return = o_realloc(to_return, (len+1)*sizeof(char));
        if (o_strlen(to_return) > 0) {
          strcat(to_return, "\n");
        }
      } else {
        to_return = o_malloc((o_strlen(line) + 1)*sizeof(char));
        to_return[0] = 0;
      }
      strcat(to_return, line);
      o_free(line);
    }
    return to_return;
  } else {
    return NULL;
  }
}

char * read_file(const char * filename) {
  char * buffer = NULL;
  long length;
  FILE * f = fopen (filename, "rb");
  if (f != NULL) {
    fseek (f, 0, SEEK_END);
    length = ftell (f);
    fseek (f, 0, SEEK_SET);
    buffer = o_malloc (length + 1);
    if (buffer != NULL) {
      fread (buffer, 1, length, f);
      buffer[length] = '\0';
    }
    fclose (f);
  }
  return buffer;
}

int main (int argc, char **argv) {
  int ret;
  
  // Set the framework port number
  struct _u_instance instance;

  //y_init_logs("simple_example", Y_LOG_MODE_CONSOLE, Y_LOG_LEVEL_DEBUG, NULL, "Starting simple_example");
  
  if (ulfius_init_instance(&instance, PORT, NULL, NULL) != U_OK) {
    // y_log_message(Y_LOG_LEVEL_ERROR, "Error ulfius_init_instance, abort");
    return(1);
  }
  
  u_map_put(instance.default_headers, "Access-Control-Allow-Origin", "*");
  
  // Maximum body size sent by the client is 1 Kb
  instance.max_post_body_size = 1024;

  // MIME types that will define the static files
  struct _u_map mime_types;
  u_map_init(&mime_types);
  u_map_put(&mime_types, ".html", "text/html");
  u_map_put(&mime_types, ".css", "text/css");
  u_map_put(&mime_types, ".js", "application/javascript");
  u_map_put(&mime_types, ".png", "image/png");
  u_map_put(&mime_types, ".jpeg", "image/jpeg");
  u_map_put(&mime_types, ".jpg", "image/jpeg");
  u_map_put(&mime_types, "*", "application/octet-stream");  
  
  // Endpoint list declaration
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, NULL, 0, &callback_get_test, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/empty", 0, &callback_get_empty_response, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/multiple/:multiple/:multiple/:not_multiple", 0, &callback_all_test_foo, NULL);
  ulfius_add_endpoint_by_val(&instance, "POST", PREFIX, NULL, 0, &callback_post_test, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/param/:foo", 0, &callback_all_test_foo, "user data 1");
  ulfius_add_endpoint_by_val(&instance, "POST", PREFIX, "/param/:foo", 0, &callback_all_test_foo, "user data 2");
  ulfius_add_endpoint_by_val(&instance, "PUT", PREFIX, "/param/:foo", 0, &callback_all_test_foo, "user data 3");
  ulfius_add_endpoint_by_val(&instance, "DELETE", PREFIX, "/param/:foo", 0, &callback_all_test_foo, "user data 4");
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIXCOOKIE, "/:lang/:extra", 0, &callback_get_cookietest, NULL);
  
  /////
  ulfius_add_endpoint_by_val(&instance, "GET", JSON1_URL, NULL, 0, &callback_json1_test, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", JSON3_URL, NULL, 0, &callback_json3_test, NULL);
  /////

  ulfius_add_endpoint_by_val(&instance, "GET", "*", NULL, 1, &callback_static_file, &mime_types);
  
  // default_endpoint declaration
  ulfius_set_default_endpoint(&instance, &callback_default, NULL);
  
  // Start the framework
  if (argc == 4 && o_strcmp("-secure", argv[1]) == 0) {
    // If command-line options are -secure <key_file> <cert_file>, then open an https connection
    char * key_pem = read_file(argv[2]), * cert_pem = read_file(argv[3]);
    ret = ulfius_start_secure_framework(&instance, key_pem, cert_pem);
    o_free(key_pem);
    o_free(cert_pem);
  } else {
    // Open an http connection
    ret = ulfius_start_framework(&instance);
  }
  
  if (ret == U_OK) {
    // y_log_message(Y_LOG_LEVEL_DEBUG, "Start %sframework on port %d", ((argc == 4 && o_strcmp("-secure", argv[1]) == 0)?"secure ":""), instance.port);
    
    // Wait for the user to press <enter> on the console to quit the application
    getchar();
  } else {
    // y_log_message(Y_LOG_LEVEL_DEBUG, "Error starting framework");
  }
  // y_log_message(Y_LOG_LEVEL_DEBUG, "End framework");
  
  // y_close_logs();
  
  ulfius_stop_framework(&instance);
  ulfius_clean_instance(&instance);
  
  return 0;
}

/**
 * Callback function that put a "Hello World!" string in the response
 */
int callback_get_test (const struct _u_request * request, struct _u_response * response, void * user_data) {
  ulfius_set_string_body_response(response, 200, "Hello World!");
  return U_CALLBACK_CONTINUE;
}

/**
 * Callback function that put an empty response and a status 200
 */
int callback_get_empty_response (const struct _u_request * request, struct _u_response * response, void * user_data) {
  return U_CALLBACK_CONTINUE;
}

/**
 * Callback function that put a "Hello World!" and the post parameters send by the client in the response
 */
int callback_post_test (const struct _u_request * request, struct _u_response * response, void * user_data) {
  char * post_params = print_map(request->map_post_body);
  char * response_body = msprintf("Hello World!\n%s", post_params);
  ulfius_set_string_body_response(response, 200, response_body);
  o_free(response_body);
  o_free(post_params);
  return U_CALLBACK_CONTINUE;
}

/**
 * Callback function that put "Hello World!" and all the data sent by the client in the response as string (http method, url, params, cookies, headers, post, json, and user specific data in the response
 */
int callback_all_test_foo (const struct _u_request * request, struct _u_response * response, void * user_data) {
  char * url_params = print_map(request->map_url), * headers = print_map(request->map_header), * cookies = print_map(request->map_cookie), 
        * post_params = print_map(request->map_post_body);
#ifndef _WIN32
  char * response_body = msprintf("Hello World!\n\n  method is %s\n  url is %s\n\n  parameters from the url are \n%s\n\n  cookies are \n%s\n\n  headers are \n%s\n\n  post parameters are \n%s\n\n  user data is %s\n\nclient address is %s\n\n",
                                  request->http_verb, request->http_url, url_params, cookies, headers, post_params, (char *)user_data, inet_ntoa(((struct sockaddr_in *)request->client_address)->sin_addr));
#else
  char * response_body = msprintf("Hello World!\n\n  method is %s\n  url is %s\n\n  parameters from the url are \n%s\n\n  cookies are \n%s\n\n  headers are \n%s\n\n  post parameters are \n%s\n\n  user data is %s\n\n",
                                  request->http_verb, request->http_url, url_params, cookies, headers, post_params, (char *)user_data);
#endif
  ulfius_set_string_body_response(response, 200, response_body);
  o_free(url_params);
  o_free(headers);
  o_free(cookies);
  o_free(post_params);
  o_free(response_body);
  return U_CALLBACK_CONTINUE;
}

/**
 * Callback function that sets cookies in the response
 * The counter cookie is incremented every time the client reloads this url
 */
int callback_get_cookietest (const struct _u_request * request, struct _u_response * response, void * user_data) {
  const char * lang = u_map_get(request->map_url, "lang"), * extra = u_map_get(request->map_url, "extra"), 
             * counter = u_map_get(request->map_cookie, "counter");
  char new_counter[8];
  int i_counter;
  
  if (counter == NULL) {
    i_counter = 0;
  } else {
    i_counter = strtol(counter, NULL, 10);
    i_counter++;
  }
  snprintf(new_counter, 7, "%d", i_counter);
  ulfius_add_cookie_to_response(response, "lang", lang, NULL, 0, NULL, NULL, 0, 0);
  ulfius_add_cookie_to_response(response, "extra", extra, NULL, 0, NULL, NULL, 0, 0);
  ulfius_add_cookie_to_response(response, "counter", new_counter, NULL, 0, NULL, NULL, 0, 0);
  ulfius_set_string_body_response(response, 200, "Cookies set!");
  
  return U_CALLBACK_CONTINUE;
}

////////////////////////////////
int callback_json1_test(const struct _u_request * request, struct _u_response * response, void * user_data) {

  char * url_params = print_map(request->map_url), * headers = print_map(request->map_header), * cookies = print_map(request->map_cookie), *post_params = print_map(request->map_post_body);

  char *response_body = NULL;
  const char **keys;
  char *valueKey;

  keys = u_map_enum_keys(request->map_url);

  if (keys[0]) {

    if (0 == o_strcmp("U0VMRUNUIFNZU1Bob25lTGFuZ3VhZ2UgRlJPTSBUQUJfU1lTVEVNX1BIT05F", keys[0])) {
        response_body = msprintf(JSON1_CONTENT);
    } else if (0 == o_strcmp("U0VMRUNUIFNZU1RpbWVNYW51YWwsU1lTVGltZURhdGUsU1lTVGltZUhvdXIsU1lTVGltZUZvcm1hdCxTWVNUaW1lRGF0ZUZvcm1hdCxTWVNUaW1lVXBkYXRlSW50ZXJ2YWwsU1lTVGltZUVuYWJsZU5UUCxTWVNUaW1lVGltZVpvbmUsU1lTVGltZURheWxpZ3RoU2F2aW5nLFNZU1RpbWVOVFBGaXJzdEFkZHJlc3MsU1lTVGltZU5UUFNlY29uZEFkZHJlc3MsaG91clN0YXJ0LGhvdXJTdG9wLGRheVN0YXJ0LGRheVN0b3Asd2Vla0RheVN0YXJ0LHdlZWtEYXlTdG9wLHdlZWtTdGFydCx3ZWVrU3RvcCxtb250aFN0YXJ0LG1vbnRoU3RvcCxvZmZzZXREYXlsaWdodFNhdmluZyBGUk9NIFRBQl9TWVNURU1fVElNRTs", keys[0])) {
      response_body = msprintf(JSON2_CONTENT);
    }
  } else {
    response_body = msprintf("INDEFINIDO");
  }

  ulfius_set_string_body_response(response, 200, response_body);
  o_free(url_params);
  o_free(headers);
  o_free(cookies);
  o_free(post_params);
  o_free(response_body);
  return U_CALLBACK_COMPLETE;
}
int callback_json2_test(const struct _u_request * request, struct _u_response * response, void * user_data) {

  char * url_params = print_map(request->map_url), * headers = print_map(request->map_header), * cookies = print_map(request->map_cookie), *post_params = print_map(request->map_post_body);

  char *response_body = msprintf(JSON2_CONTENT);

  ulfius_set_string_body_response(response, 200, response_body);
  o_free(url_params);
  o_free(headers);
  o_free(cookies);
  o_free(post_params);
  o_free(response_body);
  return U_CALLBACK_COMPLETE;
}
int callback_json3_test(const struct _u_request * request, struct _u_response * response, void * user_data) {

  char * url_params = print_map(request->map_url), * headers = print_map(request->map_header), * cookies = print_map(request->map_cookie), *post_params = print_map(request->map_post_body);

  char *response_body = msprintf(JSON3_CONTENT);
  ulfius_set_string_body_response(response, 200, response_body);
  o_free(url_params);
  o_free(headers);
  o_free(cookies);
  o_free(post_params);
  o_free(response_body);


  return U_CALLBACK_COMPLETE;
}
///////////////////////////////

/**
 * return the filename extension
 */
const char * get_filename_ext(const char *path) {
    const char *dot = o_strrchr(path, '.');
    if(!dot || dot == path) return "*";
    return dot + 1;
}

/**
 * serve a static file to the client as a very simple http server
 */
int callback_static_file (const struct _u_request * request, struct _u_response * response, void * user_data) {
  void * buffer = NULL;
  long length;
  FILE * f;
  char  * file_path = msprintf("%s%s", STATIC_FOLDER, request->http_url);
  const char * content_type;
  int bFoundFile = 0;

  if (access(file_path, F_OK) != -1) {

    if ((1 == o_strlen(request->http_url)) && (0 == o_strcmp("/", request->http_url))) {
        o_free(file_path);
        file_path = msprintf("%s%s", STATIC_FOLDER, "/index.html");
    }

    bFoundFile = 1;
  } 
  
  if (bFoundFile) {
    f = fopen (file_path, "rb");
    if (f) {
      fseek (f, 0, SEEK_END);
      length = ftell (f);
      fseek (f, 0, SEEK_SET);
      buffer = o_malloc(length*sizeof(void));
      if (buffer) {
        fread (buffer, 1, length, f);
      }
      fclose (f);
    }

    if (buffer) {
      content_type = u_map_get((struct _u_map *)user_data, get_filename_ext(file_path));
      response->binary_body = buffer;
      response->binary_body_length = length;
      u_map_put(response->map_header, "Content-Type", content_type);
      response->status = 200;
    } else {
      response->status = 404;
    }
  } else {
    response->status = 404;
  }
  o_free(file_path);
  return U_CALLBACK_CONTINUE;
}

/**
 * Default callback function called if no endpoint has a match
 */
int callback_default (const struct _u_request * request, struct _u_response * response, void * user_data) {
  ulfius_set_string_body_response(response, 404, "Page not found, do what you want");
  return U_CALLBACK_CONTINUE;
}
