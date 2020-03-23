/**************************************************************************
 *
 * system-status.c
 *
 *    Get general information to web service
 *
 * Copyright 2019 Intelbras
 *
 **************************************************************************/

#include <database.h>
#include <config.h>
#include <jansson.h>
#include <utils.h>
#include <system-status.h>
#include <netdb.h>
#include <time.h>
#include <ctype.h>
#include <ulfius.h>
#include <iniparser.h>
#include <network.h>

#ifndef	MSG_CONFIRM
  #define MSG_CONFIRM 0
#endif

#define THIS_FILE "system-status.c"

SYSTEM_GENERAL systemGeneral;
struct _db_connection *pConnDB;

static void loadSystemGeneral(SYSTEM_GENERAL *pSystemGeneral) {

  char *pchToken, pchVersion[VERSION_LENGHT];
  struct _db_result result;

	iniparser_get_config(CFG_ACCOUNTS_NUMBER, &pSystemGeneral->accountNumber, TYPE_WORD);
	iniparser_get_config(CFG_PRODUCT, &pSystemGeneral->pchProduct, TYPE_STRING);
	iniparser_get_config(CFG_PRODUCT_VERSION, &pSystemGeneral->pchVersion, TYPE_STRING);
	iniparser_get_config(CFG_BRANCH, &pSystemGeneral->pchBranch, TYPE_STRING);
	iniparser_get_config(CFG_DATABASE, &pSystemGeneral->pchDatabasePath, TYPE_STRING);
  iniparser_get_config(CFG_DEV_ID, &pSystemGeneral->dev_id, TYPE_WORD);

  o_strcpy(pchVersion, pSystemGeneral->pchVersion);
  pchToken = strtok(pchVersion, ".");
  pSystemGeneral->wMajor = atoi(pchToken);
  pchToken = strtok(NULL, ".");
  pSystemGeneral->wMinor = atoi(pchToken);
  pchToken = strtok(NULL, ".");
  pSystemGeneral->wPatch = atoi(pchToken);

  pSystemGeneral->loginTime    = time(NULL); 
  pSystemGeneral->pchAdminUser = o_strdup("admin");

  if (db_query_select(pConnDB, "SELECT SECPassword FROM TAB_SECURITY_ACCOUNT where SECAccount='admin'", &result) == DATABASE_OK) {
    pSystemGeneral->pchAdminPwd  = o_strdup(((struct _db_type_text *)result.data[0][0].t_data)->value);
  } else {
    pSystemGeneral->pchAdminPwd = o_strdup("admin");
  } 

}

void initSystemGeneral(struct _db_connection *pConn) {
  pConnDB = pConn;
  memset(&systemGeneral, 0, sizeof(SYSTEM_GENERAL));
  iniparser_open();
  loadSystemGeneral(&systemGeneral);
  iniparser_close();
}

SYSTEM_GENERAL* getSystemGeneral() {
  return &systemGeneral;
}

bool isAuthenticated(const struct _u_request *request, struct _u_response *response) {

  if (request->auth_basic_user != NULL && request->auth_basic_password != NULL && 0 == o_strcmp(request->auth_basic_user, systemGeneral.pchAdminUser) && 0 == o_strcmp(request->auth_basic_password, systemGeneral.pchAdminPwd) && (time(NULL) < (systemGeneral.loginTime + LOGIN_TIMEOUT))) {
    systemGeneral.loginTime = time(NULL);
    return true;
  } else {
    systemGeneral.loginTime = time(NULL);
    ulfius_set_string_body_response(response, 401, "Error authentication");
    u_map_put(response->map_header, "WWW-Authenticate", "Basic realm=\"IP phone Intelbras (INTELBRAS)\"");
    u_map_put(response->map_header, "Connection", "close");
    return false;
  }
}

static void getRegisterStatus(word wAccount, word *wRegisterCode, bool *bRegisterICIP) {

#ifdef __APPLE__
  *wRegisterCode = 200;
  *bRegisterICIP = 0;
#elif PLATFORM_X86  
  *wRegisterCode = 200;
  *bRegisterICIP = 0;
#else
  int sockfd, recvLen, slen; 
  char pchBuffer[BUFFER_REG_LENGHT]; 
  struct sockaddr_in servaddr; 
  char pchAccount[ACCOUNT_LENGHT];

  sprintf(pchAccount, "%d", wAccount);

  if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) { 
    log_error("socket creation failed"); 
    return; 
  } 

  memset(&servaddr, 0, sizeof(servaddr)); 
  slen = sizeof(servaddr);
  memset(pchBuffer, 0, BUFFER_REG_LENGHT);
    
  servaddr.sin_family       = AF_INET; 
  servaddr.sin_port         = htons(PORT_SERVER_REG_STATUS); 
  servaddr.sin_addr.s_addr  = INADDR_ANY; 
    
  sendto(sockfd, (const char *)pchAccount, o_strlen(pchAccount), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr)); 
  if ((recvLen = recvfrom(sockfd, (char *)pchBuffer, BUFFER_REG_LENGHT, MSG_WAITALL, (struct sockaddr *) &servaddr, &slen)) != ERROR) {
    char *pchToken = strtok(pchBuffer, ",");
    if (pchToken) {
      *wRegisterCode = atoi(pchToken);
      pchToken = strtok(NULL, ",");
      *bRegisterICIP = atoi(pchToken);
    }    
  }

  close(sockfd); 
#endif

  return;
}

bool getRegisterStatusAccount(json_t ** j_result, word wAccount) {

	int i;
  json_t *j_data;
  char pchUserField[10];
  word wRegisterCode;
  bool bRegisterICIP;

  if (j_result == NULL) {
    return false;
  }

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return false;
  } 
 
	for (i = 0; i < systemGeneral.accountNumber; i++) {
    sprintf(pchUserField, "user%d", (i+1));
    getRegisterStatus(i, &wRegisterCode, &bRegisterICIP);
    json_object_set_new(j_data, "RegisterStatus", json_integer(wRegisterCode));
    json_object_set_new(j_data, "RegisterICIP", json_integer(bRegisterICIP));
	}

  json_array_append_new(*j_result, j_data);

	return true;
}

static int getEndpointStatus() {

  int endpointStatus;
  int sockfd, recvLen, slen; 
  char pchBuffer[BUFFER_REG_LENGHT]; 
  struct sockaddr_in servaddr; 

#ifndef PLATFORM_X86

  endpointStatus = ENDPOINT_BUSY;

  if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) { 
    log_error("socket creation failed"); 
    return endpointStatus;
  } 

  memset(&servaddr, 0, sizeof(servaddr)); 
  slen = sizeof(servaddr);
  memset(pchBuffer, 0, BUFFER_REG_LENGHT);
    
  servaddr.sin_family       = AF_INET; 
  servaddr.sin_port         = htons(PORT_SERVER_ENDPOINT_STATUS); 
  servaddr.sin_addr.s_addr  = INADDR_ANY; 
    
  sendto(sockfd, ENDPOINT_STATUS, o_strlen(ENDPOINT_STATUS), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr)); 
  if ((recvLen = recvfrom(sockfd, (char *)pchBuffer, BUFFER_REG_LENGHT, MSG_WAITALL, (struct sockaddr *) &servaddr, &slen)) != ERROR) {
     endpointStatus = atoi(pchBuffer);
  }

  close(sockfd);

#else  
  endpointStatus = 1;
#endif   

  return endpointStatus;
}

bool getEndpointFreeStatus(json_t ** j_result) {

	int i;
  char pchUserField[USER_FIELD_LENGHT];
  word wRegisterCode;
  bool bRegisterICIP;

  if (j_result == NULL) {
    return false;
  }

  json_object_set_new(*j_result, "endpointsFree", json_integer(getEndpointStatus()));
  
	return true;
}

bool getStatusAccount(json_t ** j_result) {

	int i;
  json_t *j_data;
  char pchUserField[USER_FIELD_LENGHT];
  word wRegisterCode;
  bool bRegisterICIP;

  if (j_result == NULL) {
    return false;
  }
 
	for (i = 0; i < systemGeneral.accountNumber; i++) {
    sprintf(pchUserField, "user%d", (i+1));
    getRegisterStatus(i, &wRegisterCode, &bRegisterICIP);
    json_object_set_new(*j_result, pchUserField, json_integer(wRegisterCode));
	}

	return true;
}

bool getStatusNetwork(json_t **j_result) {

  json_t *j_data;
  int protocolMode;
  char *pchInterface, *pchMac, *pchIPv4, *pchIPv6, *pchMask, *pchGateway, *pchDns1, *pchDns2; 

  if (j_result == NULL) {
    return false;
  }

  j_data = *j_result;

  pchIPv4 = NULL;
  pchIPv6 = NULL;
  pchMask = NULL;
  pchDns1 = NULL;
  pchDns2 = NULL; 
  pchInterface = ntw_get_active_interface_name(pConnDB);
  protocolMode = ntw_get_protocol_mode(pConnDB);

  if ((protocolMode == 0) || (protocolMode == 2)) {

    pchIPv4    = ntw_get_if_addr(pchInterface, false);
    pchMask    = ntw_get_mac(pchInterface, true);
    pchGateway = ntw_get_if_gateway(pchInterface, false);

    json_object_set_new(j_data, "add_ipv4",     json_string(pchIPv4));
    json_object_set_new(j_data, "netmask",      json_string(pchMask));
    json_object_set_new(j_data, "gateway_ipv4", json_string(pchGateway));
    json_object_set_new(j_data, "type_ipv4",    json_integer(ntw_get_interface_type(pchInterface, false, pConnDB)));

    ntw_get_dns_servers(&pchDns1, &pchDns2, false);
    if (pchDns1) {
      json_object_set_new(j_data, "dns1_ipv4", json_string(pchDns1));
    } else {
      json_object_set_new(j_data, "dns1_ipv4", json_string(""));
    }

    if (pchDns2) {
      json_object_set_new(j_data, "dns1_ipv4", json_string(pchDns2));
    } else {
      json_object_set_new(j_data, "dns2_ipv4", json_string(""));
    }

  } else {
    json_object_set_new(j_data, "add_ipv4",     json_string(""));
    json_object_set_new(j_data, "netmask",      json_string(""));
    json_object_set_new(j_data, "gateway_ipv4", json_string(""));
    json_object_set_new(j_data, "type_ipv4",    json_string(""));
    json_object_set_new(j_data, "dns1_ipv4",    json_string(""));
    json_object_set_new(j_data, "dns2_ipv4",    json_string(""));
  }

  if ((protocolMode == 1) || (protocolMode == 2)) {
    pchIPv6    = ntw_get_if_addr(pchInterface, true);
    pchGateway = ntw_get_if_gateway(pchInterface, true);

    json_object_set_new(j_data, "add_ipv6",     json_string(pchIPv6));
    json_object_set_new(j_data, "gateway_ipv6", json_string(pchGateway));
    json_object_set_new(j_data, "type_ipv6",    json_integer(ntw_get_interface_type(pchInterface, true, pConnDB)));

    ntw_get_dns_servers(&pchDns1, &pchDns2, true);
    if (pchDns1) {
      json_object_set_new(j_data, "dns1_ipv6", json_string(pchDns1));
    } else {
      json_object_set_new(j_data, "dns1_ipv6", json_string(""));
    }

    if (pchDns2) {
      json_object_set_new(j_data, "dns2_ipv6", json_string(pchDns2));
    } else {
      json_object_set_new(j_data, "dns2_ipv6", json_string(""));
    }
   
  } else {
    json_object_set_new(j_data, "add_ipv6",     json_string(""));
    json_object_set_new(j_data, "gateway_ipv6", json_string(""));
    json_object_set_new(j_data, "type_ipv6",    json_string(""));
    json_object_set_new(j_data, "dns1_ipv6",    json_string(""));
    json_object_set_new(j_data, "dns2_ipv6",    json_string(""));
  }  

  pchMac = ntw_get_mac(DEFAULT_INTERFACE, false);
  json_object_set_new(j_data, "mac",       json_string(pchMac));
  json_object_set_new(j_data, "prot_mode", json_integer(protocolMode));

  o_free(pchInterface);
  o_free(pchMac);
  o_free(pchIPv6);
  o_free(pchMask);
  o_free(pchGateway);
  o_free(pchDns1);
  o_free(pchDns2);   

	return true;
}

bool getStatusSystem(json_t **j_result) {

  json_t *j_data;
  FILE *pf; 
  time_t seconds = time(NULL);
  struct tm tm = *localtime(&seconds);
  char pchDate[SIZE_STR_STATUS_SYS], pchCmdRet[SIZE_STR_STATUS_SYS];

  if (j_result == NULL) {
    return false;
  }

  j_data = *j_result;

  pf = popen("cat /proc/uptime", "r");
  if (pf) {

    memset(pchCmdRet, 0, SIZE_STR_STATUS_SYS);
    fgets(pchCmdRet, SIZE_STR_STATUS_SYS, pf);

    json_object_set_new(j_data, "tmp_op", json_string(pchCmdRet));

    pclose(pf);
  } 

  pf = popen("uname -n", "r");
  if (pf) {

    memset(pchCmdRet, 0, SIZE_STR_STATUS_SYS);
    fgets(pchCmdRet, SIZE_STR_STATUS_SYS, pf);

    json_object_set_new(j_data, "host_name", json_string(pchCmdRet));

    pclose(pf);
  } 

   pf = popen("fw_printenv rootfspart | cut -d\"=\" -f2'", "r");
   if (pf) {

    int partition;

    memset(pchCmdRet, 0, SIZE_STR_STATUS_SYS);
    fgets(pchCmdRet, SIZE_STR_STATUS_SYS, pf);

    if (o_strcmp(pchCmdRet, "rootfs") == 0) {
      partition = 1;
    } else {
      partition = 2;
    }

    json_object_set_new(j_data, "freePartition", json_integer(partition));

    pclose(pf);
  }  

  json_object_set_new(j_data, "tmp_ntp",  json_real(seconds));

  sprintf(pchDate, "%d/%d/%d", tm.tm_mday, tm.tm_mon, tm.tm_year);
  json_object_set_new(j_data, "date",     json_string(pchDate));

  sprintf(pchDate, "%d:%d:%d", tm.tm_hour, tm.tm_min, tm.tm_sec);
  json_object_set_new(j_data, "time24h",  json_string(pchDate));

  sprintf(pchDate, "%d:%d:%d", tm.tm_hour/12, tm.tm_min, tm.tm_sec);
  json_object_set_new(j_data, "time12h",  json_string(pchDate));

  json_object_set_new(j_data, "swMajor",  json_integer(systemGeneral.wMajor));
  json_object_set_new(j_data, "swMinor",  json_integer(systemGeneral.wMinor));  
  json_object_set_new(j_data, "swPatch",  json_integer(systemGeneral.wPatch));  

  json_object_set_new(j_data, "hwVersion", json_string("1")); // default hardware

	return true;
}

static char *getAccountsName() {

  json_t *j_data, *pResult;
  char *pchAccountsName, pchField[SIZE_STR_STATUS_SYS];
  int i, account;
  word wRegisterCode;
  bool bRegisterICIP;

  pchAccountsName = NULL;

  pResult = json_array();    
  if (pResult) {  

    j_data = json_object();
    if (j_data == NULL) {
      json_decref(pResult); 
      return false;
    }

    for (i = 0; i < systemGeneral.accountNumber; i++) {
      struct _db_result result;
      char *pchSelectAccount;

       account = i + 1;

      pchSelectAccount = msprintf("SELECT CallerIDName, PhoneNumber FROM TAB_VOIP_ACCOUNT where PK = %d", account);
      if (db_query_select(pConnDB, pchSelectAccount, &result) == DATABASE_OK) {
        if (result.nb_rows == 1) {

          char *pchCallerIDName = ((struct _db_type_text *)result.data[0][0].t_data)->value;
          char *pchPhoneNumber  = ((struct _db_type_text *)result.data[0][1].t_data)->value;

          if (pchCallerIDName) {
            sprintf(pchField, "username_%d", account);
            json_object_set_new(j_data, pchField, json_string(pchCallerIDName));
          }

          if (pchPhoneNumber) {
            sprintf(pchField, "extension_%d", account);
            json_object_set_new(j_data, pchField, json_string(pchPhoneNumber));
          }                  
        }
      }
      o_free(pchSelectAccount); 

      
      pchSelectAccount = msprintf("SELECT AccountActive FROM TAB_TEL_ACCOUNT where PK = %d;", account);
      if (db_query_select(pConnDB, pchSelectAccount, &result) == DATABASE_OK) {
        if (result.nb_rows == 1) {

          char *pchEnable = ((struct _db_type_text *)result.data[0][0].t_data)->value;

          if (pchEnable) {
            sprintf(pchField, "enable_%d", account);
            json_object_set_new(j_data, pchField, json_integer(atoi(pchEnable)));
          }                    
        }
      }
      o_free(pchSelectAccount);          
      
      sprintf(pchField, "register_%d", account);
      getRegisterStatus(i, &wRegisterCode, &bRegisterICIP);
      json_object_set_new(j_data, pchField, (wRegisterCode==200)?json_true():json_false());
    }

    json_array_append_new(pResult, j_data);

    pchAccountsName = json_dumps(pResult, JSON_INDENT(2));

    json_decref(pResult); 
  }

  return pchAccountsName;
}

bool getGeneralStatus(json_t **j_result) {

  json_t *j_data;
  FILE *pf;
  char *pchMac, *pchIPv4, *pchAccountsName; 

  if (j_result == NULL) {
    return false;
  }

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return false;
  } 

  pchMac = ntw_get_mac(DEFAULT_INTERFACE, false);
  json_object_set_new(j_data, "mac",      json_string(pchMac));
  json_object_set_new(j_data, "version",  json_string(systemGeneral.pchVersion));
  if (systemGeneral.pchBranch) {
    char pchModel[MODEL_NAME_LENGHT];
    sprintf(pchModel, "%s_%s", systemGeneral.pchProduct, systemGeneral.pchBranch);
    json_object_set_new(j_data, "model",  json_string(pchModel));
  } else {
    json_object_set_new(j_data, "model",  json_string(systemGeneral.pchProduct));
  }
  json_object_set_new(j_data, "numAcc",   json_integer(systemGeneral.accountNumber));

  pchIPv4 = ntw_get_if_addr(DEFAULT_INTERFACE, false);
  json_object_set_new(j_data, "ip_address", json_string(pchIPv4));

  pchAccountsName = getAccountsName();
  json_object_set_new(j_data, "accounts", json_string(pchAccountsName));

  o_free(pchMac);
  o_free(pchIPv4);
  o_free(pchAccountsName);

  json_array_append_new(*j_result, j_data);

	return true;
}

bool getGigaSupport(json_t **j_result) {

  json_t *j_data;
  FILE *pf;
  char *pchMac, *pchIPv4, *pchAccountsName; 

  if (j_result == NULL) {
    return false;
  }

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return false;
  } 

  json_object_set_new(j_data, "supportGiga", json_string("0")); // Disable giga support

  json_array_append_new(*j_result, j_data);

	return true;
}

void convertToUpperCase(char *pchSrc, char *pchDest) {

  while (*pchSrc != '\0') {
    *pchDest = toupper(*pchSrc);
    pchSrc++;
    pchDest++;
  }

  *pchDest = POINTER_NULL;
}

bool getVersionStatus(json_t **j_result) {

  json_t *j_data;
  FILE *pf;
  char pchUpper[PRODUCT_NAME_LENGHT]; 

  if (j_result == NULL) {
    return false;
  }

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return false;
  } 
  
  json_object_set_new(j_data, "version", json_string(systemGeneral.pchVersion));
  convertToUpperCase(systemGeneral.pchProduct, pchUpper);
  json_object_set_new(j_data, "product", json_string(pchUpper));
  if (systemGeneral.pchBranch) {
    convertToUpperCase(systemGeneral.pchBranch, pchUpper);
    json_object_set_new(j_data, "branch", json_string(pchUpper));
  }

  json_array_append_new(*j_result, j_data);

	return true;
}

bool getFwCloudVersion(json_t **j_result) {

  json_t *j_data;
  FILE *pf;
  char *pchVersionLatest = NULL; 

  if (j_result == NULL) {
    return false;
  }

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return false;
  } 
  
  // pchVersionLatest = getStatusLatest(); // TODO 
  if (pchVersionLatest) {
    json_object_set_new(j_data, "captureFileFail",  json_false());
    json_object_set_new(j_data, "versionLatest",    json_string(pchVersionLatest));
    json_object_set_new(j_data, "actualFwVersion",  json_string(systemGeneral.pchVersion));
  } else {
    json_object_set_new(j_data, "captureFileFail",  json_true());
  }
  
  json_array_append_new(*j_result, j_data);

	return true;
}
