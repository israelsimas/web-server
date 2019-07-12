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
#include <system-request.h>
#include <middleware.h>
#include <file-process.h>

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

  connDB = h_connect_sqlite(DATABASE_PATH);
  if (!connDB) {
    LOG_ERROR("Database unreacheable");
    return(1);
  }

  initSystemGeneral(connDB);
  initFileProcess(connDB);

  openMiddleware();
  
  u_map_put(instance.default_headers, "Access-Control-Allow-Origin", "*");
  
  if (ulfius_set_upload_file_callback_function(&instance, &file_upload_callback, "my cls") != U_OK) {
    LOG_ERROR("Error ulfius_set_upload_file_callback_function");
  }

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
  ulfius_add_endpoint_by_val(&instance, "GET", STATUS_GENERAL_REQUEST, NULL, 0, &callback_status_general, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", SUPPORT_GIGA_REQUEST, NULL, 0, &callback_support_giga, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", VERSIO_REQUEST, NULL, 0, &callback_version, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", NOTIFY_REQUEST, NULL, 0, &callback_notify, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", AUTOPROV_LOG_REQUEST, NULL, 0, &callback_autoprov_log, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", DATE_TIME_REQUEST, NULL, 0, &callback_date_time, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", SELF_PROV_REQUEST, NULL, 0, &callback_self_provisioning, NULL);  
  ulfius_add_endpoint_by_val(&instance, "GET", CHANGE_PARTITION_REQUEST, NULL, 0, &callback_change_partition, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", CAPTURE_LOG_REQUEST, NULL, 0, &callback_capture_log, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", STATUS_FW_CLOUD_REQUEST, NULL, 0, &callback_status_fw_cloud, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", UPDATE_FW_CLOUD_REQUEST, NULL, 0, &callback_update_fw_cloud, NULL);  
  ulfius_add_endpoint_by_val(&instance, "GET", BACKUP_REQUEST, NULL, 0, &callback_backup, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", SAVE_FW_REQUEST, NULL, 0, &callback_save_fw, NULL); 
  ulfius_add_endpoint_by_val(&instance, "GET", BURN_FW_REQUEST, NULL, 0, &callback_burn_fw, NULL);  
  ulfius_add_endpoint_by_val(&instance, "GET", END_FW_REQUEST, NULL, 0, &callback_end_fw, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", BURN_STATUS_REQUEST, NULL, 0, &callback_burn_status, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", EXPORT_AUTOPROV_XML, NULL, 0, &callback_export_autoprov, NULL);  
  ulfius_add_endpoint_by_val(&instance, "POST", RESTART_REQUEST, NULL, 0, &callback_restart, NULL);
  ulfius_add_endpoint_by_val(&instance, "POST", RESTART_SYSLOG_REQUEST, NULL, 0, &callback_restart_syslog, NULL);
  ulfius_add_endpoint_by_val(&instance, "POST", FACTORY_RESET_REQUEST, NULL, 0, &callback_factory_reset, NULL);
  ulfius_add_endpoint_by_val(&instance, "POST", LOGO_RESET_REQUEST, NULL, 0, &callback_logo_reset, NULL);
  ulfius_add_endpoint_by_val(&instance, "POST", SET_LANGUAGE_REQUEST, NULL, 0, &callback_set_language, NULL); 
  ulfius_add_endpoint_by_val(&instance, "POST", UPLOAD_CONFIG_REQUEST, NULL, 1, &callback_upload_file, NULL); 
  ulfius_add_endpoint_by_val(&instance, "GET", "*", NULL, 1, &callback_static_file, &mime_types);
  
  // default_endpoint declaration
  ulfius_set_default_endpoint(&instance, &callback_default, NULL);

  if (SECURE_CONNECTION) {
    // Open an https connection
    char *key_pem = readFile(CERTIFICATE_KEY), *cert_pem = readFile(CERTIFICATE_PEM);
    status = ulfius_start_secure_framework(&instance, key_pem, cert_pem);
    o_free(key_pem);
    o_free(cert_pem);
  } else {
    // Open an http connection
    status = ulfius_start_framework(&instance);
  }

  if (status == SUCCESS) {
    
    // Wait for the user to press <enter> on the console to quit the application
    while (TRUE) {
      sleep(WAIT_MAIN_LOOP);
    }
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

  char *pchResponseBody = NULL;
  json_t *pResultAcc, *pResultNetwork, *pResultSystem;
  char *pchResult, *pchResultAcc, *pchResultNetwork, *pchResultSystem, *result;
  SYSTEM_GENERAL *pSystemStatus;

  pSystemStatus = getSystemGeneral();

  pResultAcc = json_object();
  getStatusAccount(&pResultAcc);
  pchResultAcc = json_dumps(pResultAcc, JSON_INDENT(2));

  pResultNetwork = json_object();
  getStatusNetwork(&pResultNetwork);
  pchResultNetwork = json_dumps(pResultNetwork, JSON_INDENT(2));

  pResultSystem = json_object();
  getStatusSystem(&pResultSystem);
  pchResultSystem = json_dumps(pResultSystem, JSON_INDENT(2));

  pchResponseBody = msprintf("{ \"account\": %s,\n \"net\": %s,\n \"system\": %s,\n \"product\":\"%s\",\n \"branch\":\"%s\" }", pchResultAcc, pchResultNetwork, pchResultSystem, pSystemStatus->pchProduct, pSystemStatus->pchBranch);

  o_free(pchResultAcc);
  o_free(pchResultNetwork);
  o_free(pchResultSystem);

  json_decref(pResultAcc);
  json_decref(pResultNetwork);
  json_decref(pResultSystem);

  ulfius_set_string_body_response(response, HTTP_SC_OK, pchResponseBody);
  o_free(pchResponseBody);

  return U_CALLBACK_COMPLETE;
}

int callback_endpoint_status(const struct _u_request *request, struct _u_response *response, void *user_data) {

  json_t *pResult;
  char *pchResponseBody;

  pResult = json_object();
  if (pResult) {  
    getEndpointFreeStatus(&pResult);
    pchResponseBody = json_dumps(pResult, JSON_INDENT(2));
    json_decref(pResult); 

    ulfius_set_string_body_response(response, HTTP_SC_OK, pchResponseBody);

    o_free(pchResponseBody);

    return U_CALLBACK_COMPLETE;
  } else {
    return U_CALLBACK_CONTINUE;
  }
}

int callback_status_register(const struct _u_request *request, struct _u_response *response, void *user_data) {

  json_t *pResult;
  char *pchResponseBody;
  const char **ppKeys;
  WORD wAccount;

  ppKeys = u_map_enum_keys(request->map_url);

  if (ppKeys[0]) {
    wAccount = atoi(ppKeys[0]);
    pResult = json_array();    
    if (pResult) {  
      getRegisterStatusAccount(&pResult, wAccount);
      pchResponseBody = json_dumps(pResult, JSON_INDENT(2));
      json_decref(pResult); 

      ulfius_set_string_body_response(response, HTTP_SC_OK, pchResponseBody);

      o_free(pchResponseBody);

      return U_CALLBACK_COMPLETE;
    } else {
      return U_CALLBACK_CONTINUE;
    }
  } else {
      return U_CALLBACK_CONTINUE;
  }  
}

int callback_status_general(const struct _u_request *request, struct _u_response *response, void *user_data) {

  json_t *pResult;
  char *pchResponseBody;

  pResult = json_array();    
  if (pResult) {  
    getGeneralStatus(&pResult);
    pchResponseBody = json_dumps(pResult, JSON_INDENT(2));
    json_decref(pResult); 

    ulfius_set_string_body_response(response, HTTP_SC_OK, pchResponseBody);

    o_free(pchResponseBody);

    return U_CALLBACK_COMPLETE;
  } else {
    return U_CALLBACK_CONTINUE;
  } 
}

int callback_support_giga(const struct _u_request *request, struct _u_response *response, void *user_data) {

  json_t *pResult;
  char *pchResponseBody;

  pResult = json_array();    
  if (pResult) {  
    getGigaSupport(&pResult);
    pchResponseBody = json_dumps(pResult, JSON_INDENT(2));
    json_decref(pResult); 

    ulfius_set_string_body_response(response, HTTP_SC_OK, pchResponseBody);

    o_free(pchResponseBody);

    return U_CALLBACK_COMPLETE;
  } else {
    return U_CALLBACK_CONTINUE;
  } 
}

int callback_version(const struct _u_request *request, struct _u_response *response, void *user_data) {

  json_t *pResult;
  char *pchResponseBody;

  pResult = json_array();    
  if (pResult) {  
    getVersionStatus(&pResult);
    pchResponseBody = json_dumps(pResult, JSON_INDENT(2));
    json_decref(pResult); 

    ulfius_set_string_body_response(response, HTTP_SC_OK, pchResponseBody);

    o_free(pchResponseBody);

    return U_CALLBACK_COMPLETE;
  } else {
    return U_CALLBACK_CONTINUE;
  } 
}

int callback_notify(const struct _u_request *request, struct _u_response *response, void *user_data) {

  const char **ppKeys;
  const char *pchValue;
  WORD wAccount;

  ppKeys = u_map_enum_keys(request->map_url);

  if (ppKeys[0]) {
    pchValue = u_map_get(request->map_url, ppKeys[0]);
    notifyTables(pchValue);
    ulfius_set_string_body_response(response, HTTP_SC_OK, NULL);

    return U_CALLBACK_COMPLETE;  
  } else {
    return U_CALLBACK_CONTINUE;
  } 
}

int callback_autoprov_log(const struct _u_request *request, struct _u_response *response, void *user_data) {

  // TODO - log to autoprov not available
  ulfius_set_string_body_response(response, HTTP_SC_NOT_FOUND, NULL);

  return U_CALLBACK_COMPLETE; 
}

int callback_date_time(const struct _u_request *request, struct _u_response *response, void *user_data) {

  const char **ppKeys;
  const char *pchValue;

  ppKeys = u_map_enum_keys(request->map_url);

  if (ppKeys[0] && (!o_strcmp(ppKeys[0], "updateNTP"))) {
    setNTPDateTime();
    ulfius_set_string_body_response(response, HTTP_SC_OK, NULL);
    return U_CALLBACK_COMPLETE;  
  } else if (ppKeys[0] && (!o_strcmp(ppKeys[0], "datetime"))) {
    pchValue = u_map_get(request->map_url, ppKeys[0]);
    if (pchValue) {
      setManualDateTime(pchValue);
    }
    ulfius_set_string_body_response(response, HTTP_SC_OK, NULL);
    return U_CALLBACK_COMPLETE;      
  } else {
    return U_CALLBACK_CONTINUE;
  }   
}

int callback_self_provisioning(const struct _u_request *request, struct _u_response *response, void *user_data) {

  if (request->map_url) {
    setSelfProvision(request->map_url);
    ulfius_set_string_body_response(response, HTTP_SC_OK, NULL);
    return U_CALLBACK_COMPLETE;  
  } else {
    return U_CALLBACK_CONTINUE;
  }
}

int callback_change_partition(const struct _u_request *request, struct _u_response *response, void *user_data) {

  const char **ppKeys;
  const char *pchValue;

  ppKeys = u_map_enum_keys(request->map_url);

  if (ppKeys[0]) {
    pchValue = u_map_get(request->map_url, ppKeys[0]);
    if (pchValue) {
      setChangeBootPartition(atoi(pchValue));
      ulfius_set_string_body_response(response, HTTP_SC_OK, NULL);
    } else {
      ulfius_set_string_body_response(response, HTTP_SC_INTERNAL_SERVER_ERROR, NULL);
    }
    return U_CALLBACK_COMPLETE;  
  } else {
    return U_CALLBACK_CONTINUE;
  } 
}

int callback_capture_log(const struct _u_request *request, struct _u_response *response, void *user_data) {

  const char **ppKeys;
  const char *pchValue;
  ppKeys = u_map_enum_keys(request->map_url);

  if (ppKeys[0] && (!o_strcmp(ppKeys[0], "action"))) {
    pchValue = u_map_get(request->map_url, ppKeys[0]);
    if (pchValue) {
      if (!o_strcmp(pchValue, "start")) {
        startCaptureLog();
        ulfius_set_string_body_response(response, HTTP_SC_OK, NULL);
      } else if (!o_strcmp(pchValue, "stop")) {
        stopCaptureLog();
        ulfius_set_string_body_response(response, HTTP_SC_OK, NULL);
      } else if (!o_strcmp(pchValue, "export")) {

        void *pchBuffer = NULL;
        long length;
        FILE *pFile = NULL;

        system("cd /data/logs/ && tar -cvf /data/logs/logs.tar *");

        if (access("/data/logs/logs.tar", SUCCESS) != ERROR) {
          pFile = fopen ("/data/logs/logs.tar", "rb");
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

            u_map_put(response->map_header, "Content-Type", "application/octet-stream");
            u_map_put(response->map_header, "Content-Disposition", "Attachment;filename=logs.tar");
            
            response->binary_body        = pchBuffer;
            response->binary_body_length = length;
            
            response->status = HTTP_SC_OK;
          } else {
            response->status = HTTP_SC_NOT_FOUND;
          }
        }
        ulfius_set_string_body_response(response, HTTP_SC_OK, NULL);
      } else {
        ulfius_set_string_body_response(response, HTTP_SC_INTERNAL_SERVER_ERROR, NULL);
      }

    } else {
      ulfius_set_string_body_response(response, HTTP_SC_INTERNAL_SERVER_ERROR, NULL);
    }
    return U_CALLBACK_COMPLETE;  
  } else {
    return U_CALLBACK_CONTINUE;
  } 
}

int callback_status_fw_cloud(const struct _u_request *request, struct _u_response *response, void *user_data) {

  json_t *pResult;
  char *pchResponseBody;

  pResult = json_array();    
  if (pResult) {  
    getFwCloudVersion(&pResult);
    pchResponseBody = json_dumps(pResult, JSON_INDENT(2));
    json_decref(pResult); 

    ulfius_set_string_body_response(response, HTTP_SC_OK, pchResponseBody);

    o_free(pchResponseBody);

    return U_CALLBACK_COMPLETE;
  } else {
    return U_CALLBACK_CONTINUE;
  } 
}

int callback_update_fw_cloud(const struct _u_request *request, struct _u_response *response, void *user_data) {

  json_t *pResult;
  char *pchResponseBody;

  pResult = json_array();    
  if (pResult) {  
    // getVersionStatus(&pResult); TODO
    // pchResponseBody = json_dumps(pResult, JSON_INDENT(2));
    json_decref(pResult); 

    ulfius_set_string_body_response(response, HTTP_SC_OK, NULL);

    o_free(pchResponseBody);

    return U_CALLBACK_COMPLETE;
  } else {
    return U_CALLBACK_CONTINUE;
  } 
} 

int callback_restart(const struct _u_request *request, struct _u_response *response, void *user_data) {

  restartSystem();
  ulfius_set_string_body_response(response, HTTP_SC_OK, NULL);

  return U_CALLBACK_COMPLETE;
}

int callback_restart_syslog(const struct _u_request *request, struct _u_response *response, void *user_data) {

  restartSyslog();
  ulfius_set_string_body_response(response, HTTP_SC_OK, NULL);

  return U_CALLBACK_COMPLETE;
}

int callback_factory_reset(const struct _u_request *request, struct _u_response *response, void *user_data) {

  factoryReset();
  ulfius_set_string_body_response(response, HTTP_SC_OK, NULL);

  return U_CALLBACK_COMPLETE;
}

int callback_logo_reset(const struct _u_request *request, struct _u_response *response, void *user_data) {

  logoReset();
  ulfius_set_string_body_response(response, HTTP_SC_OK, NULL);

  return U_CALLBACK_COMPLETE;
}

int callback_set_language(const struct _u_request *request, struct _u_response *response, void *user_data) {

  setLanguage();
  ulfius_set_string_body_response(response, HTTP_SC_OK, NULL);

  return U_CALLBACK_COMPLETE;
}

int callback_save_fw(const struct _u_request *request, struct _u_response *response, void *user_data) {

  ulfius_set_string_body_response(response, HTTP_SC_OK, NULL);

  return U_CALLBACK_COMPLETE;  
}

int callback_burn_fw(const struct _u_request *request, struct _u_response *response, void *user_data) {

  char *pchResponseBody = NULL;
  int status;

  status = getFirmwareStatus();
  switch(status) {

    case FIRMWARE_VALID:
      updateFirmware();
      pchResponseBody = msprintf("Gravando Firmware");
      break;
    
    case FIRMWARE_INVALID_FILE:
      pchResponseBody = msprintf("Arquivo de Firmware Inválido");
      break;

    case FIRMWARE_INVALID_PRODUCT:
      pchResponseBody = msprintf("Arquivo de Firmware Inválido para este produto");
      break;

    case FIRMWARE_INVALID_VERSION:
      pchResponseBody = msprintf("Mesma versão");
      break;

    default:
      pchResponseBody = msprintf("Arquivo de Firmware Inválido");
      break; 
  }  

  ulfius_set_string_body_response(response, HTTP_SC_OK, pchResponseBody);

  o_free(pchResponseBody);

  return U_CALLBACK_COMPLETE;
}

int callback_end_fw(const struct _u_request *request, struct _u_response *response, void *user_data) {

  char *pchResponseBody = NULL;

  closeFwupdate(&pchResponseBody);
  ulfius_set_string_body_response(response, HTTP_SC_OK, pchResponseBody);

  return U_CALLBACK_COMPLETE;  
}

int callback_burn_status(const struct _u_request *request, struct _u_response *response, void *user_data) {

  json_t *pResult;
  char *pchResponseBody;
 
  pResult = json_object();   
  if (pResult) {  
    getBurningStatus(pResult);
    pchResponseBody = json_dumps(pResult, JSON_INDENT(2));
    json_decref(pResult); 

    ulfius_set_string_body_response(response, HTTP_SC_OK, pchResponseBody);

    o_free(pchResponseBody);

    return U_CALLBACK_COMPLETE;
  } else {
    return U_CALLBACK_CONTINUE;
  }  
}

int callback_export_autoprov(const struct _u_request *request, struct _u_response *response, void *user_data) {

  char *pchBuffer;
  int lenBuffer;

  u_map_put(response->map_header, "Content-Type", "application/xml");
  u_map_put(response->map_header, "Content-Disposition", "Attachment;filename=Autoprovisioning.xml");
  
  lenBuffer = getAutoprovXML(&pchBuffer);
  response->binary_body        = pchBuffer;
  response->binary_body_length = lenBuffer;
  
  response->status = HTTP_SC_OK;

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

int callback_backup(const struct _u_request *request, struct _u_response *response, void *user_data) {

  void *pchBuffer = NULL;
  long length;
  FILE *pFile = NULL;

  system("rm /data/databaseCipher.sql");
  system("openssl enc -aes-256-cbc -salt -in /data/database.sql -out /data/databaseCipher.sql -k SIRIUS_INTELBRAS");

  if (access("/data/databaseCipher.sql", SUCCESS) != ERROR) {
    pFile = fopen ("/data/databaseCipher.sql", "rb");
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

      u_map_put(response->map_header, "Content-Type", "application/octet-stream");
      u_map_put(response->map_header, "Content-Disposition", "Attachment;filename=config.db");
      
      response->binary_body        = pchBuffer;
      response->binary_body_length = length;
      
      response->status = HTTP_SC_OK;
    } else {
      response->status = HTTP_SC_NOT_FOUND;
    }
  }
  
  return U_CALLBACK_CONTINUE;
}

char * print_map(const struct _u_map * map) {
  char * line, * to_return = NULL;
  const char **keys, * value;
  int len, i;
  if (map != NULL) {
    keys = u_map_enum_keys(map);
    for (i=0; keys[i] != NULL; i++) {
      value = u_map_get(map, keys[i]);
      len = snprintf(NULL, 0, "key is %s, length is %zu, value is %.*s", keys[i], u_map_get_length(map, keys[i]), (int)u_map_get_length(map, keys[i]), value);
      line = o_malloc((len+1)*sizeof(char));
      snprintf(line, (len+1), "key is %s, length is %zu, value is %.*s", keys[i], u_map_get_length(map, keys[i]), (int)u_map_get_length(map, keys[i]), value);
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

int callback_upload_file (const struct _u_request * request, struct _u_response * response, void * user_data) {

  const char **ppKeys;
  const char *pchValue;
  char *pchResponseBody = NULL;

  ppKeys = u_map_enum_keys(request->map_url);

  if (ppKeys[0] && !o_strcmp(ppKeys[0], "action")) {
    pchValue = u_map_get(request->map_url, ppKeys[0]);

    if (!o_strcmp(pchValue, "importConfig")) {
      closeUploadFile(UPLOAD_FILE_CONFIG);
      updateConfig();
    } else if (!o_strcmp(pchValue, "importLogo")) {
      closeUploadFile(UPLOAD_FILE_LOGO);
      updateLogo();
    } else if (!o_strcmp(pchValue, "importContacts")) {
      closeUploadFile(UPLOAD_FILE_LOGO);
      updateContacts();
    } else if (!o_strcmp(pchValue, "importPatch")) {
      closeUploadFile(UPLOAD_FILE_PATCH);
      updatePatch();
    } else if (!o_strcmp(pchValue, "addRing")) {
      const char **ppKeysRing;

      closeUploadFile(UPLOAD_FILE_RING);
      ppKeysRing = u_map_enum_keys(request->map_post_body);
      if (ppKeysRing[1] && (request->map_post_body->nb_values == 2) && !o_strcmp(ppKeysRing[1], "SYSRingName")) {
        pchValue = u_map_get(request->map_post_body, ppKeysRing[1]);
        updateRing(pchValue);
      }
    } else if (!o_strcmp(pchValue, "loadFirmware")) {
      
      closeUploadFile(UPLOAD_FILE_FIRMWARE);

    } else {
      LOG_ERROR("Invalid file");
    }
  }

  ulfius_set_string_body_response(response, 200, pchResponseBody);
  o_free(pchResponseBody);

  return U_CALLBACK_COMPLETE;
}

int file_upload_callback (const struct _u_request * request, 
                          const char * key, 
                          const char * filename, 
                          const char * content_type, 
                          const char * transfer_encoding, 
                          const char * data, 
                          uint64_t off, 
                          size_t size, 
                          void * cls) {

  const char **ppKeys;

  if (request->http_url && o_strstr(request->http_url, "action")) {

    if (o_strstr(request->http_url, "importConfig")) {
      loadUploadFile(data, off, size, UPLOAD_FILE_CONFIG);
    } else if (o_strstr(request->http_url, "importLogo")) {
      loadUploadFile(data, off, size, UPLOAD_FILE_LOGO);
     } else if (o_strstr(request->http_url, "importContacts")) {
      loadUploadFile(data, off, size, UPLOAD_FILE_CONTACTS);     
    } else if (o_strstr(request->http_url, "importPatch")) {
      loadUploadFile(data, off, size, UPLOAD_FILE_PATCH);
    } else if (o_strstr(request->http_url, "addRing")) {
      loadUploadFile(data, off, size, UPLOAD_FILE_RING);
    } else if (o_strstr(request->http_url, "loadFirmware")) {
      loadUploadFile(data, off, size, UPLOAD_FILE_FIRMWARE);
    } else {
      LOG_ERROR("Invalid file");
    }
  }

  return U_OK;
}

int callback_default(const struct _u_request *request, struct _u_response *response, void *user_data) {

  ulfius_set_string_body_response(response, HTTP_SC_NOT_FOUND, NOT_FOUND_MESSAGE);
  
  return U_CALLBACK_CONTINUE;
}
