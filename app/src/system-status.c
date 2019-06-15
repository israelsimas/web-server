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

  o_strcpy(pchVersion, pSystemGeneral->pchVersion);
  pchToken = strtok(pchVersion, ".");
  pSystemGeneral->pchswMajor = o_strdup(pchToken);
  pchToken = strtok(NULL, ".");
  pSystemGeneral->swMinor = o_strdup(pchToken);
  pchToken = strtok(NULL, ".");
  pSystemGeneral->swPatch = o_strdup(pchToken);
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


static void getRegisterStatus(WORD wAccount, WORD *wRegisterCode, BOOL *bRegisterICIP) {

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
  int sockfd;
	char buffer[BUFFER_ENDP_LENGHT];
	int numSend, numRcv, len;
	struct sockaddr_un remote;
  int endpointStatus = ENDPOINT_BUSY;

	/*--- Open socket ---*/
	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		printf("Socket");
		return endpointStatus;
	}

  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, SOCK_STATUS_ADDR);
  len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	/*---Connect to server---*/
  if (connect(sockfd, (struct sockaddr *)&remote, len) != 0) {
		printf("Connect ");
		return endpointStatus;
	}

	 /* Send message to the server */
	numSend = send(sockfd, ENDPOINT_STATUS, sizeof(ENDPOINT_STATUS), 0);
	if (numSend < 0) {
		printf("ERROR writing to socket");
		return endpointStatus;
	}

	/* Data from Server */
	bzero(buffer, BUFFER_ENDP_LENGHT);
	numRcv = recv(sockfd, buffer, sizeof(buffer), 0);
	if (numRcv > 0) {
		if (!strcmp(buffer, "0")) {
			endpointStatus = 0;
		} else {
			endpointStatus = 1;
		}
	}

	/*---Clean up---*/
	close(sockfd);

	return endpointStatus;
}

BOOL getEndpointFreeStatus(json_t ** j_result) {

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

  json_object_set_new(j_data, "endpointsFree", json_integer(getEndpointStatus()));
  
  json_array_append_new(*j_result, j_data);

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

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return FALSE;
  } 
 
	for (i = 0; i < systemGeneral.accountNumber; i++) {
    sprintf(pchUserField, "user%d", (i+1));
    getRegisterStatus(i, &wRegisterCode, &bRegisterICIP);
    json_object_set_new(j_data, pchUserField, json_integer(wRegisterCode));
	}

  json_array_append_new(*j_result, j_data);

	return TRUE;
}

static char *getActiveInterface() {
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

static char *getMac() {

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

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return FALSE;
  } 

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

  json_array_append_new(*j_result, j_data);

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

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return FALSE;
  } 

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

  json_object_set_new(j_data, "swMajor", json_string(systemGeneral.pchswMajor));
  json_object_set_new(j_data, "swMinor", json_string(systemGeneral.swMinor));  
  json_object_set_new(j_data, "swPatch", json_string(systemGeneral.swPatch));  

  json_object_set_new(j_data, "hwVersion", json_string("1")); // default hardware

  json_array_append_new(*j_result, j_data);

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

BOOL getBurningStatus(json_t **j_result) {
  
  json_t *j_data;
  FILE *pf;
  char pchPercent[PERCENTAGE_STR_LEN];

  if (j_result == NULL) {
    return FALSE;
  }

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return FALSE;
  } 

  sprintf(pchPercent, "%d", 0);
  pf = popen("cat /tmp/burningPercent", "r");
	if (pf) {
		fgets(pchPercent, PERCENTAGE_STR_LEN, pf);
		pclose(pf);
	} 

  json_object_set_new(j_data, "percent", json_integer(atoi(pchPercent)));

  json_array_append_new(*j_result, j_data);

	return TRUE; 
}
