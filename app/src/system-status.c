/**************************************************************************
 *
 * system-status.c
 *
 *    Get general information to web service
 *
 * Copyright 2019 Intelbras
 *
 **************************************************************************/

#include <base64.h>
#include <misc.h>
#include <hoel.h>
#include <config.h>
#include <jansson.h>
#include <misc.h>
#include <system-status.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <ulfius.h>
#include <time.h> 

#define MSG_CONFIRM 0

#define THIS_FILE "system-status.c"

SYSTEM_GENERAL systemGeneral;
struct _h_connection *pConnDB;

static void loadSystemGeneral(SYSTEM_GENERAL *pSystemGeneral) {

  char *pchToken, pchVersion[10];

	getConfig(CFG_ACCOUNTS_NUMBER, &pSystemGeneral->accountNumber, TYPE_WORD);
	getConfig(CFG_PRODUCT, &pSystemGeneral->pchProduct, TYPE_STRING);
	getConfig(CFG_PRODUCT_VERSION, &pSystemGeneral->pchVersion, TYPE_STRING);
	getConfig(CFG_BRANCH, &pSystemGeneral->pchBranch, TYPE_STRING);
	getConfig(CFG_DATABASE, &pSystemGeneral->pchDatabasePath, TYPE_STRING);
  getConfig(CFG_DEV_ID, &pSystemGeneral->dev_id, TYPE_WORD);

  o_strcpy(pchVersion, pSystemGeneral->pchVersion);
  pchToken = strtok(pchVersion, ".");
  pSystemGeneral->wMajor = atoi(pchToken);
  pchToken = strtok(NULL, ".");
  pSystemGeneral->wMinor = atoi(pchToken);
  pchToken = strtok(NULL, ".");
  pSystemGeneral->wPatch = atoi(pchToken);

  pSystemGeneral->pchAdminUser = o_strdup("admin");
  pSystemGeneral->pchAdminPwd  = o_strdup("admin");
  pSystemGeneral->loginTime    = time(NULL); 
}

void initSystemGeneral(struct _h_connection *pConn) {
  memset(&systemGeneral, 0, sizeof(SYSTEM_GENERAL));
  openConfig();
  loadSystemGeneral(&systemGeneral);
  closeConfig();
  pConnDB = pConn;
}

SYSTEM_GENERAL* getSystemGeneral() {
  return &systemGeneral;
}

BOOL isAuthenticated(const struct _u_request *request, struct _u_response *response) {

  if (request->auth_basic_user != NULL && request->auth_basic_password != NULL && 0 == o_strcmp(request->auth_basic_user, systemGeneral.pchAdminUser) && 0 == o_strcmp(request->auth_basic_password, systemGeneral.pchAdminPwd) && (time(NULL) < (systemGeneral.loginTime + LOGIN_TIMEOUT))) {
    systemGeneral.loginTime = time(NULL);
    return TRUE;
  } else {
    systemGeneral.loginTime = time(NULL);
    ulfius_set_string_body_response(response, 401, "Error authentication");
    u_map_put(response->map_header, "WWW-Authenticate", "Basic realm=\"IP phone Intelbras (INTELBRAS)\"");
    u_map_put(response->map_header, "Connection", "close");
    return FALSE;
  }
}

static void getRegisterStatus(WORD wAccount, WORD *wRegisterCode, BOOL *bRegisterICIP) {

#if 1
  int sockfd, recvLen, slen; 
  char pchBuffer[BUFFER_REG_LENGHT]; 
  struct sockaddr_in servaddr; 
  char pchAccount[3];

  sprintf(pchAccount, "%d", wAccount);

  if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) { 
    LOG_ERROR("socket creation failed"); 
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
#else
  *wRegisterCode = 200;
  *bRegisterICIP = 0;
#endif

  return;
}

BOOL getRegisterStatusAccount(json_t ** j_result, WORD wAccount) {

	int i;
  json_t *j_data;
  char pchUserField[10];
  WORD wRegisterCode;
  BOOL bRegisterICIP;

  if (j_result == NULL) {
    return FALSE;
  }

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return FALSE;
  } 
 
	for (i = 0; i < systemGeneral.accountNumber; i++) {
    sprintf(pchUserField, "user%d", (i+1));
    getRegisterStatus(i, &wRegisterCode, &bRegisterICIP);
    json_object_set_new(j_data, "RegisterStatus", json_integer(wRegisterCode));
    json_object_set_new(j_data, "RegisterICIP", json_integer(bRegisterICIP));
	}

  json_array_append_new(*j_result, j_data);

	return TRUE;
}

static int getEndpointStatus() {
  int endpointStatus = ENDPOINT_BUSY;
  int sockfd, recvLen, slen; 
  char pchBuffer[BUFFER_REG_LENGHT]; 
  struct sockaddr_in servaddr; 

  if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) { 
    LOG_ERROR("socket creation failed"); 
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

  return endpointStatus;

}

BOOL getEndpointFreeStatus(json_t ** j_result) {

	int i;
  char pchUserField[10];
  WORD wRegisterCode;
  BOOL bRegisterICIP;

  if (j_result == NULL) {
    return FALSE;
  }

  json_object_set_new(*j_result, "endpointsFree", json_integer(getEndpointStatus()));
  
	return TRUE;
}

BOOL getStatusAccount(json_t ** j_result) {

	int i;
  json_t *j_data;
  char pchUserField[10];
  WORD wRegisterCode;
  BOOL bRegisterICIP;

  if (j_result == NULL) {
    return FALSE;
  }
 
	for (i = 0; i < systemGeneral.accountNumber; i++) {
    sprintf(pchUserField, "user%d", (i+1));
    getRegisterStatus(i, &wRegisterCode, &bRegisterICIP);
    json_object_set_new(*j_result, pchUserField, json_integer(wRegisterCode));
	}

	return TRUE;
}

char *getActiveInterface() {
  char *pchInterface = NULL;
  struct _h_result result;

  if (h_query_select(pConnDB, "SELECT VLANActivate, VLANID, VLANAutoEnable, VLANAutoConfigured, VLANAutoID FROM TAB_NET_VLAN", &result) == H_OK) {
    if (result.nb_rows == 1 && result.nb_columns == 5) {

      if (((struct _h_type_int *)result.data[0][0].t_data)->value == 1) {
        pchInterface = msprintf("%s.%d", DEFAULT_INTERFACE, ((struct _h_type_int *)result.data[0][1].t_data)->value);
      } else if ( (((struct _h_type_int *)result.data[0][2].t_data)->value == 1) && (((struct _h_type_int *)result.data[0][3].t_data)->value == 1)) {
        pchInterface = msprintf("%s.%d", DEFAULT_INTERFACE, ((struct _h_type_int *)result.data[0][4].t_data)->value);
      }
    }
  }

  if (!pchInterface) {
    pchInterface = o_strdup(DEFAULT_INTERFACE);
  }

  return pchInterface;
}

static int getProtocolMode() {

  int protocolMode = 0;
  struct _h_result result;

  if (h_query_select(pConnDB, "SELECT ETHProtocolMode FROM TAB_NET_ETH_WAN", &result) == H_OK) {
    if (result.nb_rows == 1 && result.nb_columns == 1) {
      protocolMode = ((struct _h_type_int *)result.data[0][0].t_data)->value;
    }
  }

  return protocolMode;
}

static char *getIfaddr(char *pchIfName, int typeINET) {
	struct ifaddrs *ifap, *ifa;
  char addr[INET6_ADDRSTRLEN];
  char *ret_addr;

  getifaddrs (&ifap);
  for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
    if ((o_strcmp(ifa->ifa_name, pchIfName) == 0) && (ifa->ifa_addr->sa_family == typeINET)) {

      if (typeINET == AF_INET) {
        getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), addr, sizeof(addr), NULL, 0, NI_NUMERICHOST);
      } else {
        getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6), addr, sizeof(addr), NULL, 0, NI_NUMERICHOST);
      }    

      ret_addr = o_strdup(addr);
      break;
    }
  }
  freeifaddrs(ifap);

	if (ret_addr)
	  return ret_addr;

	return o_strdup(INVALID_IP);
}

static char *getMaskaddr(char *pchIfName, int typeINET) {
	struct ifaddrs *ifap, *ifa;
  char *pchAddr = NULL;
  char *ret_addr;

  getifaddrs (&ifap);
  for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
    if ((o_strcmp(ifa->ifa_name, pchIfName) == 0) && (ifa->ifa_addr->sa_family == typeINET)) {

      if (typeINET == AF_INET) {
        char *pchMask;
        struct sockaddr_in *sa = (struct sockaddr_in *) ifa->ifa_netmask;
        pchMask = inet_ntoa(sa->sin_addr);
        if (pchMask) {
          pchAddr = o_strdup(pchMask);
        }
      } else {
        char pchMask[50];
        struct sockaddr_in6 *sa = (struct sockaddr_in6 *) ifa->ifa_netmask;

        memset(pchMask, 0, 50);
        inet_ntop(AF_INET6, (void *) &sa->sin6_addr, pchMask, (sizeof(char) * 50));
        if (o_strlen(pchMask)) {
          pchAddr = o_strdup(pchMask);
        }        
      }
      break;
    }
  }
  freeifaddrs(ifap);

	if (!pchAddr)
	  pchAddr = o_strdup("255.255.255.0");

	return pchAddr;
}

static int getInterfaceType(char *pchIfName, BOOL isIPv6) {

  char *pchQuery;
  char *pchTable;
  char *pchParamDHCP;
  int interfaceType = 0;
  struct _h_result result;

  if (!o_strcmp(pchIfName, DEFAULT_INTERFACE)) {
    pchTable = "TAB_NET_ETH_WAN";
    if (isIPv6) {
      pchParamDHCP = "ETHActivateDHCPClientIPv6";      
    } else {
      pchParamDHCP = "ETHActivateDHCPClient";
    }
  } else {
    pchTable = "TAB_NET_VLAN";
    if (isIPv6) {
      pchParamDHCP = "VLANActivateDHCPClientIPv6";      
    } else {
      pchParamDHCP = "VLANActivateDHCPClient"; 
    }
  }

  pchQuery = msprintf("SELECT GROUP_CONCAT( %s, ',' ) as dhcp FROM %s", pchParamDHCP, pchTable);
  if (h_query_select(pConnDB, pchQuery, &result) == H_OK) {
    if (result.nb_rows == 1 && result.nb_columns == 1) {
      interfaceType = ((struct _h_type_int *)result.data[0][0].t_data)->value;
    }
  } 

  o_free(pchQuery);

  return interfaceType;
}

static char *getIfGateway(char *pchIfName, BOOL isIPv6) {

  char *pchGateway = NULL;
	FILE *pf;

  if (isIPv6) {
    char *pchComand = msprintf("/sbin/ifconfig %s | ip -6 addr | grep 'inet6 ' | grep 'scope link' | head -1 | awk '{ print $2}'", pchIfName);
    pf = popen(pchComand, "r");
    o_free(pchComand);    
  } else {
    pf = popen("/sbin/ip route list table default | awk '/default/ { print $3 }'", "r");
  }

	if (pf) {

		pchGateway = malloc(sizeof(char) * SIZE_STR_GATEWAY);
		memset(pchGateway, 0, SIZE_STR_GATEWAY);
		fgets(pchGateway, SIZE_STR_GATEWAY, pf);

		pclose(pf);
	} 

  if (!pchGateway) {
    pchGateway = o_strdup("255.255.255.0");  
  }  

  return pchGateway;
}

char *getMac() {

  char *pchMAC = NULL;
	FILE *pf;

  pf = popen("/sbin/ifconfig eth0 | grep -o -E '([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}'", "r");
	if (pf) {

		pchMAC = malloc(sizeof(char) * SIZE_STR_MAC);
		memset(pchMAC, 0, SIZE_STR_MAC);
		fgets(pchMAC, SIZE_STR_MAC, pf);

		pclose(pf);
	} 

  if (!pchMAC) {
    pchMAC = o_strdup("00:00:00:00:00:00");  
  }  

  return pchMAC;
}

void get_dns_servers(char **ppchDns1, char **ppchDns2, BOOL isIPv6) {
  FILE *pf;
  char line[200] , *pchDNS;
  BOOL bDNS1 = TRUE;

  if (isIPv6) {
    pf = popen("cat /etc/resolvIPv6.conf", "r");  
  } else {
    pf = popen("cat /etc/resolvIPv4.conf", "r");
  }

  if (!pf) {
    return;
  }

  while (fgets(line , 200 , pf)) {

    if(line[0] == '#') {
      continue;
    }

    if (o_strncmp(line , "nameserver" , o_strlen("nameserver")) == 0) {

      pchDNS = strtok(line , " ");
      pchDNS = strtok(NULL , " ");

      if (pchDNS && bDNS1) {
        *ppchDns1 = o_strdup(pchDNS);
        bDNS1 = FALSE;
      } else if (pchDNS) {
        *ppchDns2 = o_strdup(pchDNS);
      }
    }
  }

  pclose(pf);
}

BOOL getStatusNetwork(json_t **j_result) {

  json_t *j_data;
  int protocolMode;
  char *pchInterface, *pchMac, *pchIPv4, *pchIPv6, *pchMask, *pchGateway, *pchDns1, *pchDns2; 

  if (j_result == NULL) {
    return FALSE;
  }

  j_data = *j_result;

  pchIPv4 = NULL;
  pchIPv6 = NULL;
  pchMask = NULL;
  pchDns1 = NULL;
  pchDns2 = NULL; 
  pchInterface = getActiveInterface();
  protocolMode = getProtocolMode();

  if ((protocolMode == 0) || (protocolMode == 2)) {

    pchIPv4    = getIfaddr(pchInterface, AF_INET);
    pchMask    = getMaskaddr(pchInterface, AF_INET);
    pchGateway = getIfGateway(pchInterface, FALSE);

    json_object_set_new(j_data, "add_ipv4", json_string(pchIPv4));
    json_object_set_new(j_data, "netmask", json_string(pchMask));
    json_object_set_new(j_data, "gateway_ipv4", json_string(pchGateway));
    json_object_set_new(j_data, "type_ipv4", json_integer(getInterfaceType(pchInterface, FALSE)));

    get_dns_servers(&pchDns1, &pchDns2, FALSE);
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
    json_object_set_new(j_data, "add_ipv4", json_string(""));
    json_object_set_new(j_data, "netmask", json_string(""));
    json_object_set_new(j_data, "gateway_ipv4", json_string(""));
    json_object_set_new(j_data, "type_ipv4", json_string(""));
    json_object_set_new(j_data, "dns1_ipv4", json_string(""));
    json_object_set_new(j_data, "dns2_ipv4", json_string(""));
  }

  if ((protocolMode == 1) || (protocolMode == 2)) {
    pchIPv6    = getIfaddr(pchInterface, AF_INET6);
    pchGateway = getIfGateway(pchInterface, TRUE);

    json_object_set_new(j_data, "add_ipv6", json_string(pchIPv6));
    json_object_set_new(j_data, "gateway_ipv6", json_string(pchGateway));
    json_object_set_new(j_data, "type_ipv6", json_integer(getInterfaceType(pchInterface, TRUE)));

    get_dns_servers(&pchDns1, &pchDns2, TRUE);
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
    json_object_set_new(j_data, "add_ipv6", json_string(""));
    json_object_set_new(j_data, "gateway_ipv6", json_string(""));
    json_object_set_new(j_data, "type_ipv6", json_string(""));
    json_object_set_new(j_data, "dns1_ipv6", json_string(""));
    json_object_set_new(j_data, "dns2_ipv6", json_string(""));
  }  

  pchMac = getMac();
  json_object_set_new(j_data, "mac", json_string(pchMac));
  json_object_set_new(j_data, "prot_mode", json_integer(protocolMode));

  o_free(pchInterface);
  o_free(pchMac);
  o_free(pchIPv6);
  o_free(pchMask);
  o_free(pchGateway);
  o_free(pchDns1);
  o_free(pchDns2);   

	return TRUE;
}

BOOL getStatusSystem(json_t **j_result) {

  json_t *j_data;
  FILE *pf; 
  time_t seconds = time(NULL);
  struct tm tm = *localtime(&seconds);
  char pchDate[SIZE_STR_STATUS_SYS];
  char pchCmdRet[SIZE_STR_STATUS_SYS];

  if (j_result == NULL) {
    return FALSE;
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

  json_object_set_new(j_data, "tmp_ntp", json_real(seconds));

  sprintf(pchDate, "%d/%d/%d", tm.tm_mday, tm.tm_mon, tm.tm_year);
  json_object_set_new(j_data, "date", json_string(pchDate));

  sprintf(pchDate, "%d:%d:%d", tm.tm_hour, tm.tm_min, tm.tm_sec);
  json_object_set_new(j_data, "time24h", json_string(pchDate));

  sprintf(pchDate, "%d:%d:%d", tm.tm_hour/12, tm.tm_min, tm.tm_sec);
  json_object_set_new(j_data, "time12h", json_string(pchDate));

  json_object_set_new(j_data, "swMajor", json_integer(systemGeneral.wMajor));
  json_object_set_new(j_data, "swMinor", json_integer(systemGeneral.wMinor));  
  json_object_set_new(j_data, "swPatch", json_integer(systemGeneral.wPatch));  

  json_object_set_new(j_data, "hwVersion", json_string("1")); // default hardware

	return TRUE;
}

static char *getAccountsName() {

  json_t *j_data;
  json_t *pResult;
  char *pchAccountsName, pchField[SIZE_STR_STATUS_SYS];
  int i, account;
  WORD wRegisterCode;
  BOOL bRegisterICIP;

  pchAccountsName = NULL;

  pResult = json_array();    
  if (pResult) {  

    j_data = json_object();
    if (j_data == NULL) {
      json_decref(pResult); 
      return FALSE;
    }

    for (i = 0; i < systemGeneral.accountNumber; i++) {
      struct _h_result result;
      char *pchSelectAccount;

       account = i + 1;

      pchSelectAccount = msprintf("SELECT CallerIDName, PhoneNumber FROM TAB_VOIP_ACCOUNT where PK = %d", account);
      if (h_query_select(pConnDB, pchSelectAccount, &result) == H_OK) {
        if (result.nb_rows == 1) {

          char *pchCallerIDName = ((struct _h_type_text *)result.data[0][0].t_data)->value;
          char *pchPhoneNumber  = ((struct _h_type_text *)result.data[0][1].t_data)->value;

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
      if (h_query_select(pConnDB, pchSelectAccount, &result) == H_OK) {
        if (result.nb_rows == 1) {

          char *pchEnable = ((struct _h_type_text *)result.data[0][0].t_data)->value;

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

BOOL getGeneralStatus(json_t **j_result) {

  json_t *j_data;
  FILE *pf;
  char *pchMac, *pchIPv4, *pchAccountsName; 

  if (j_result == NULL) {
    return FALSE;
  }

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return FALSE;
  } 

  pchMac = getMac();
  json_object_set_new(j_data, "mac", json_string(pchMac));
  json_object_set_new(j_data, "version", json_string(systemGeneral.pchVersion));
  if (systemGeneral.pchBranch) {
    char pchModel[20];
    sprintf(pchModel, "%s_%s", systemGeneral.pchProduct, systemGeneral.pchBranch);
    json_object_set_new(j_data, "model", json_string(pchModel));
  } else {
    json_object_set_new(j_data, "model", json_string(systemGeneral.pchProduct));
  }
  json_object_set_new(j_data, "numAcc", json_integer(systemGeneral.accountNumber));

  pchIPv4 = getIfaddr(DEFAULT_INTERFACE, AF_INET);
  json_object_set_new(j_data, "ip_address", json_string(pchIPv4));

  pchAccountsName = getAccountsName();
  json_object_set_new(j_data, "accounts", json_string(pchAccountsName));

  o_free(pchMac);
  o_free(pchIPv4);
  o_free(pchAccountsName);

  json_array_append_new(*j_result, j_data);

	return TRUE;
}

BOOL getGigaSupport(json_t **j_result) {

  json_t *j_data;
  FILE *pf;
  char *pchMac, *pchIPv4, *pchAccountsName; 

  if (j_result == NULL) {
    return FALSE;
  }

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return FALSE;
  } 

  json_object_set_new(j_data, "supportGiga", json_string("0")); // Disable giga support

  json_array_append_new(*j_result, j_data);

	return TRUE;
}

void convertToUpperCase(char *pchSrc, char *pchDest) {
  while (*pchSrc != '\0') {
    *pchDest = toupper(*pchSrc);
    pchSrc++;
    pchDest++;
  }

  *pchDest = POINTER_NULL;
}

BOOL getVersionStatus(json_t **j_result) {

  json_t *j_data;
  FILE *pf;
  char pchUpper[10]; 

  if (j_result == NULL) {
    return FALSE;
  }

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return FALSE;
  } 
  
  json_object_set_new(j_data, "version", json_string(systemGeneral.pchVersion));
  convertToUpperCase(systemGeneral.pchProduct, pchUpper);
  json_object_set_new(j_data, "product", json_string(pchUpper));
  if (systemGeneral.pchBranch) {
    convertToUpperCase(systemGeneral.pchBranch, pchUpper);
    json_object_set_new(j_data, "branch", json_string(pchUpper));
  }

  json_array_append_new(*j_result, j_data);

	return TRUE;
}

BOOL getFwCloudVersion(json_t **j_result) {


  json_t *j_data;
  FILE *pf;
  char *pchVersionLatest = NULL; 

  if (j_result == NULL) {
    return FALSE;
  }

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return FALSE;
  } 
  
  // pchVersionLatest = getStatusLatest(); // TODO 
  if (pchVersionLatest) {
    json_object_set_new(j_data, "captureFileFail", json_false());
    json_object_set_new(j_data, "versionLatest", json_string(pchVersionLatest));
    json_object_set_new(j_data, "actualFwVersion", json_string(systemGeneral.pchVersion));
  } else {
    json_object_set_new(j_data, "captureFileFail", json_true());
  }
  
  json_array_append_new(*j_result, j_data);

	return TRUE;
}
