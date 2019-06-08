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
#include <defTypes.h>
#include <web-server.h>

#define THIS_FILE "web-server.c"

char *readFile(const char *pchFilename) {

  char *pchBuffer = NULL;
  long length;
  FILE *pFile = fopen(pchFilename, "rb");

  if (pFile != NULL) {
    fseek(pFile, 0, SEEK_END);
    length = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);
    pchBuffer = o_malloc(length + 1);
  
    if (pchBuffer != NULL) {
      fread(pchBuffer, 1, length, pFile);
      pchBuffer[length] = POINTER_NULL;
    }
  
    fclose(pFile);
  }

  return pchBuffer;
}

int main(int argc, char **argv) {

  int status;
  
  // Set the framework port number
  struct _u_instance instance;

  if (ulfius_init_instance(&instance, PORT_DEFAULT, NULL, NULL) != SUCCESS) {
    return(1);
  }
  
  u_map_put(instance.default_headers, "Access-Control-Allow-Origin", "*");
  
  // Maximum body size sent by the client is 1 Kb
  instance.max_post_body_size = POST_SIZE_MAX;

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
  ulfius_add_endpoint_by_val(&instance, "GET", DATABASE_REQUEST,  NULL, 0, &callback_database, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", STATUS_REQUEST,    NULL, 0, &callback_status, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", "*", NULL, 1, &callback_static_file, &mime_types);
  
  // default_endpoint declaration
  ulfius_set_default_endpoint(&instance, &callback_default, NULL);
  
  // Start the framework
  if (argc == 4 && o_strcmp("-secure", argv[1]) == 0) {
    // If command-line options are -secure <key_file> <cert_file>, then open an https connection
    char *pchKeyPem = readFile(argv[2]), *pchCertPem = readFile(argv[3]);

    status = ulfius_start_secure_framework(&instance, pchKeyPem, pchCertPem);
    o_free(pchKeyPem);
    o_free(pchCertPem);

  } else {
    // Open an http connection
    status = ulfius_start_framework(&instance);
  }
  
  if (status == SUCCESS) {
    
    // Wait for the user to press <enter> on the console to quit the application
    getchar();
  } else {
    LOG_ERROR("Error starting framework");
  }
   
  ulfius_stop_framework(&instance);
  ulfius_clean_instance(&instance);
  
  return 0;
}

int callback_database(const struct _u_request *request, struct _u_response *response, void *user_data) {

  char *pchResponseBody = NULL;
  const char **ppKeys;

  ppKeys = u_map_enum_keys(request->map_url);

  if (ppKeys[0]) {

    if (0 == o_strcmp("U0VMRUNUIFNZU1Bob25lTGFuZ3VhZ2UgRlJPTSBUQUJfU1lTVEVNX1BIT05F", ppKeys[0])) {
      pchResponseBody = msprintf(JSON1_CONTENT);
    } else if (0 == o_strcmp("U0VMRUNUIFNZU1RpbWVNYW51YWwsU1lTVGltZURhdGUsU1lTVGltZUhvdXIsU1lTVGltZUZvcm1hdCxTWVNUaW1lRGF0ZUZvcm1hdCxTWVNUaW1lVXBkYXRlSW50ZXJ2YWwsU1lTVGltZUVuYWJsZU5UUCxTWVNUaW1lVGltZVpvbmUsU1lTVGltZURheWxpZ3RoU2F2aW5nLFNZU1RpbWVOVFBGaXJzdEFkZHJlc3MsU1lTVGltZU5UUFNlY29uZEFkZHJlc3MsaG91clN0YXJ0LGhvdXJTdG9wLGRheVN0YXJ0LGRheVN0b3Asd2Vla0RheVN0YXJ0LHdlZWtEYXlTdG9wLHdlZWtTdGFydCx3ZWVrU3RvcCxtb250aFN0YXJ0LG1vbnRoU3RvcCxvZmZzZXREYXlsaWdodFNhdmluZyBGUk9NIFRBQl9TWVNURU1fVElNRTs", ppKeys[0])) {
      pchResponseBody = msprintf(JSON2_CONTENT);
    }
  } else {
    pchResponseBody = msprintf("Indefined");
  }

  ulfius_set_string_body_response(response, HTTP_SC_OK, pchResponseBody);
  o_free(pchResponseBody);

  return U_CALLBACK_COMPLETE;
}

int callback_status(const struct _u_request *request, struct _u_response *response, void *user_data) {

  char *pchResponseBody = msprintf(STATUS_CONTENT);

  ulfius_set_string_body_response(response, HTTP_SC_OK, pchResponseBody);
  o_free(pchResponseBody);

  return U_CALLBACK_COMPLETE;
}

const char *getFilenameExt(const char *pchPath) {

  const char *pchDot = o_strrchr(pchPath, '.');

  if (!pchDot || pchDot == pchPath) {
    return "*";
  } else {
    return pchDot + 1;
  }
}

int callback_static_file(const struct _u_request *request, struct _u_response *response, void *user_data) {

  void *pchBuffer = NULL;
  long length;
  FILE *pFile = NULL;
  char *pchFilePath = msprintf(TEMPLATE_FILE_NAME_PATH, STATIC_FOLDER, request->http_url);
  const char *pchContentType;
  BOOL bFoundFile = FALSE;

  if (access(pchFilePath, SUCCESS) != ERROR) {

    if ((1 == o_strlen(request->http_url)) && (0 == o_strcmp("/", request->http_url))) {
      o_free(pchFilePath);
      pchFilePath = msprintf(TEMPLATE_FILE_NAME_PATH, STATIC_FOLDER, INDEX_HTML);
    }

    bFoundFile = TRUE;
  } 
  
  if (bFoundFile) {

    pFile = fopen (pchFilePath, "rb");
    if (pFile) {
      fseek(pFile, 0, SEEK_END);
      length = ftell(pFile);
      fseek(pFile, 0, SEEK_SET);
      pchBuffer = o_malloc(length*sizeof(void));
      if (pchBuffer) {
        fread(pchBuffer, 1, length, pFile);
      }
      fclose(pFile);
    }

    if (pchBuffer) {
      pchContentType = u_map_get((struct _u_map *)user_data, getFilenameExt(pchFilePath));
      response->binary_body        = pchBuffer;
      response->binary_body_length = length;
      u_map_put(response->map_header, "Content-Type", pchContentType);
      response->status = HTTP_SC_OK;
    } else {
      response->status = HTTP_SC_NOT_FOUND;
    }
  } else {
    response->status = HTTP_SC_NOT_FOUND;
  }
  
  o_free(pchFilePath);

  return U_CALLBACK_CONTINUE;
}

int callback_default(const struct _u_request *request, struct _u_response *response, void *user_data) {

  ulfius_set_string_body_response(response, HTTP_SC_NOT_FOUND, NOT_FOUND_MESSAGE);
  
  return U_CALLBACK_CONTINUE;
}
