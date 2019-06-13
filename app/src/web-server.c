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
#include <base64.h>
#include <misc.h>
#include <hoel.h>
#include <config.h>
#include <jansson.h>
#include <system-status.h>

#define THIS_FILE "web-server.c"

struct _h_connection *connDB;
char *pchWhitelist[NUMBER_WHITE_LIST_COMMANDS] = {"select", "update", "insert", "delete", "begin transaction", "end transaction", "commit"};

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
  SYSTEM_GENERAL *pSystemStatus;
  
  // Set the framework port number
  struct _u_instance instance;

  if (ulfius_init_instance(&instance, PORT_DEFAULT, NULL, NULL) != SUCCESS) {
    return(1);
  }

  initSystemGeneral();
  pSystemStatus = getSystemGeneral();

  ////////////////////////////
  {
    json_t *pResult;
    char *pchResult, *result;
    json_t *json;

    pResult = json_array();
    if (pResult) { 
      getStatusAccount(&pResult);
      getStatusNetwork(&pResult);
      getStatusSystem(&pResult);
      pchResult = json_dumps(pResult, JSON_INDENT(2));

      printf("pchResult: %s\n", pchResult);    
    }
  }
  ////////////////////////////
  
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
  ulfius_add_endpoint_by_val(&instance, "GET", ENDPOINT_STATUS_REQUEST, NULL, 0, &callback_endpoint_status, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", STATUS_REGISTER_REQUEST, NULL, 0, &callback_status_register, NULL);  
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

  connDB = h_connect_sqlite(DATABASE_PATH);
  if (!connDB) {
    LOG_ERROR("Database unreacheable");
    return(1);
  }
  
  if (status == SUCCESS) {
    
    // Wait for the user to press <enter> on the console to quit the application
    getchar();
  } else {
    LOG_ERROR("Error starting framework");
  }
   
  ulfius_stop_framework(&instance);
  ulfius_clean_instance(&instance);
  h_close_db(connDB);
  
  return 0;
}

void print_result(struct _h_result result) {
  int col, row, i;
  printf("rows: %u, col: %u\n", result.nb_rows, result.nb_columns);
  for (row = 0; row<result.nb_rows; row++) {
    for (col=0; col<result.nb_columns; col++) {
      switch(result.data[row][col].type) {
        case HOEL_COL_TYPE_INT:
          printf("| %d ", ((struct _h_type_int *)result.data[row][col].t_data)->value);
          break;
        case HOEL_COL_TYPE_DOUBLE:
          printf("| %f ", ((struct _h_type_double *)result.data[row][col].t_data)->value);
          break;
        case HOEL_COL_TYPE_TEXT:
          printf("| %s ", ((struct _h_type_text *)result.data[row][col].t_data)->value);
          break;
        case HOEL_COL_TYPE_BLOB:
          for (i=0; i<((struct _h_type_blob *)result.data[row][col].t_data)->length; i++) {
            printf("%c", *((char*)(((struct _h_type_blob *)result.data[row][col].t_data)->value+i)));
            if (i%80 == 0 && i>0) {
              printf("\n");
            }
          }
          break;
        case HOEL_COL_TYPE_NULL:
          printf("| null ");
          break;
      }
    }
    printf("|\n");
  }
}

static BOOL isValidComand(const char *pchQuery) {
  
  int i;

  for (i = 0; i < NUMBER_WHITE_LIST_COMMANDS; i++) {
    if (o_strcasestr(pchQuery, pchWhitelist[i])) {
      return TRUE;
    }
  }

  return FALSE;
}

static BOOL validCommands(char *pchQuerys[], int lenQuerys) {

  int i;

  if (lenQuerys == 0) {
    return FALSE;
  }

  for (i =0; i< lenQuerys; i++) {
    if (!isValidComand(pchQuerys[i])) {
      return FALSE;
    }
  }

  return TRUE;
}

static void setQuerys(char *pchQueryList, char *pchQuerys[], int *pLenQuerys) {

  char *pchToken = NULL;
  int wNumQuerys = 0;
  char *pchList = NULL;

  *pLenQuerys = 0;

  if (!pchQueryList) {
    return;
  }

  pchList = msprintf(pchQueryList);

  	/* Parser para recuperar tabelas individualemnte */
	pchToken = strtok(pchList, ";");
	while ((pchToken != NULL) && (wNumQuerys < NUM_MAX_QUERY_COMANDS)) {
    pchQuerys[wNumQuerys] = msprintf("%s;", pchToken);
		wNumQuerys++;
		pchToken = strtok(NULL, ";");
	}

  o_free(pchList);

  *pLenQuerys = wNumQuerys;
}

static char *concatResultsToString(json_t *pListResult[], int lenQuerys) {

  int i, lenTotal = 0;
  char *pchresults[NUM_MAX_QUERY_COMANDS];  
  char *pchResultJson = NULL;

  if (!lenQuerys) {
    return NULL;
  }

  for (i = 0; i < lenQuerys; i++) {
    pchresults[i] = json_dumps(pListResult[i], JSON_INDENT(2));
    lenTotal += o_strlen(pchresults[i]);
  }

  pchResultJson = o_malloc(lenTotal + HEADER_JSON_DB);
  memset(pchResultJson, 0, (lenTotal + HEADER_JSON_DB));

  strcpy(pchResultJson, "[\n ");
  for (i = 0; i < lenQuerys; i++) {
    if (i > 0) {
      strcat(pchResultJson, ",\n");
    }
    strcat(pchResultJson, "{\n   \"rows\": \n");
    strcat(pchResultJson, pchresults[i]);
    strcat(pchResultJson, "\n }");
  }
  strcat(pchResultJson, "\n ]");

  for (i = 0; i < lenQuerys; i++) {
    o_free(pchresults[i]);
  }

  return pchResultJson;
}

int callback_database(const struct _u_request *request, struct _u_response *response, void *user_data) {

  char *pchResponseBody = NULL;
  const char **ppKeys;
  char *pchQuerys[NUM_MAX_QUERY_COMANDS];
  json_t *pListResult[NUM_MAX_QUERY_COMANDS] = { NULL };
  int lenQuerys;

  ppKeys = u_map_enum_keys(request->map_url);

  if (ppKeys[0]) {

    char *pchQueryDecode;

    pchQueryDecode = b64_decode(ppKeys[0], strlen(ppKeys[0]));
    setQuerys(pchQueryDecode, pchQuerys, &lenQuerys);
    o_free(pchQueryDecode);

    if (validCommands(pchQuerys, lenQuerys)) {            
      int status, i;
      BOOL bValidresults = TRUE;

      for (i = 0; i < lenQuerys; i++) {
        status = h_execute_query_json(connDB, pchQuerys[i], &pListResult[i]); 
        if (status != SUCCESS) {      
          bValidresults = FALSE;
          LOG_ERROR("Invalid command to database: %s", pchQuerys[i]);
          break;
        }     
      }

      if (bValidresults) {
        pchResponseBody = concatResultsToString(pListResult, lenQuerys);
      }
      
    } else {
      LOG_ERROR("Invalid commands to database");
    }

    if (lenQuerys > 0) {
      int i;
      for (i = 0; i < lenQuerys; i++) {
        o_free(pchQuerys[i]);
        json_decref(pListResult[i]);
      }
    }
  } 

  if (pchResponseBody) {
    ulfius_set_string_body_response(response, HTTP_SC_OK, pchResponseBody);
  } else {
    pchResponseBody = msprintf("Indefined");
  }
  
  o_free(pchResponseBody);  

  return U_CALLBACK_COMPLETE;
}

int callback_status(const struct _u_request *request, struct _u_response *response, void *user_data) {

  char *pchResponseBody = msprintf(STATUS_CONTENT);

  ulfius_set_string_body_response(response, HTTP_SC_OK, pchResponseBody);
  o_free(pchResponseBody);

  return U_CALLBACK_COMPLETE;
}

int callback_endpoint_status(const struct _u_request *request, struct _u_response *response, void *user_data) {

  char *pchResponseBody = msprintf(ENDPOINT_CONTENT);

  ulfius_set_string_body_response(response, HTTP_SC_OK, pchResponseBody);
  o_free(pchResponseBody);

  return U_CALLBACK_COMPLETE;
}

int callback_status_register(const struct _u_request *request, struct _u_response *response, void *user_data) {

  char *pchResponseBody = msprintf(REGISTER_STATUS_CONTENT);

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

BOOL isGzip(const char *pchPath) {

  const char *pchDot = o_strstr(pchPath, ".gz");

  if (pchDot) {
    return TRUE;
  } else {
    return FALSE;
  }
}

int callback_static_file(const struct _u_request *request, struct _u_response *response, void *user_data) {

  void *pchBuffer = NULL;
  long length;
  FILE *pFile = NULL;
  char *pchFilePath = msprintf(TEMPLATE_FILE_NAME_GZIP_PATH, STATIC_FOLDER, request->http_url);
  const char *pchContentType;
  BOOL bFoundFile = FALSE;

  if (access(pchFilePath, SUCCESS) != ERROR) { // gzip text
    bFoundFile = TRUE;
  } else { // plain text
    
    o_free(pchFilePath);
    pchFilePath = msprintf(TEMPLATE_FILE_NAME_PATH, STATIC_FOLDER, request->http_url);
    if (access(pchFilePath, SUCCESS) != ERROR) {

      if ((1 == o_strlen(request->http_url)) && (0 == o_strcmp("/", request->http_url))) {
        o_free(pchFilePath);
        pchFilePath = msprintf(TEMPLATE_FILE_NAME_PATH, STATIC_FOLDER, INDEX_HTML);
      }

      bFoundFile = TRUE;
    }
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
      
      if (isGzip(pchFilePath)) {
        u_map_put(response->map_header, "Content-Encoding", "gzip");
      } else {
        pchContentType = u_map_get((struct _u_map *)user_data, getFilenameExt(pchFilePath));
      }
      
      response->binary_body        = pchBuffer;
      response->binary_body_length = length;
      
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
