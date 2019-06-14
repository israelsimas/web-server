/**************************************************************************
 *
 * system-status.c
 *
 *    Get general information to web service
 *
 * Copyright 2017 Intelbras
 *
 **************************************************************************/

#include <base64.h>
#include <misc.h>
#include <hoel.h>
#include <config.h>
#include <jansson.h>
#include <system-status.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#define THIS_FILE "system-status.c"

SYSTEM_GENERAL systemGeneral;
struct _h_connection *pConnDB;

static void loadSystemGeneral(SYSTEM_GENERAL *pSystemGeneral) {

	getConfig(CFG_ACCOUNTS_NUMBER, &pSystemGeneral->accountNumber, TYPE_WORD);
	getConfig(CFG_PRODUCT, &pSystemGeneral->pchProduct, TYPE_STRING);
	getConfig(CFG_PRODUCT_VERSION, &pSystemGeneral->pchVersion, TYPE_STRING);
	getConfig(CFG_BRANCH, &pSystemGeneral->pchBranch, TYPE_STRING);
	getConfig(CFG_DATABASE, &pSystemGeneral->pchDatabasePath, TYPE_STRING);
}

void initSystemGeneral(struct _h_connection *pConn) {
  openConfig();
  loadSystemGeneral(&systemGeneral);
  closeConfig();
  pConnDB = pConn;
}

SYSTEM_GENERAL* getSystemGeneral() {
  return &systemGeneral;
}

static char *getUserAccount(WORD wAccount) {

  return "200";
}

BOOL getStatusAccount(json_t ** j_result) {

	int i;
  json_t *j_data;
  char pchUserField[10];

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
    json_object_set_new(j_data, pchUserField, json_string(getUserAccount(i)));
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
  char *addr = NULL;
  char *ret_addr;

  getifaddrs (&ifap);
  for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
    if ((o_strcmp(ifa->ifa_name, pchIfName) == 0) && (ifa->ifa_addr->sa_family == typeINET)) {

      if (typeINET == AF_INET) {
        struct sockaddr_in *sa = (struct sockaddr_in *) ifa->ifa_netmask;
        addr = inet_ntoa(sa->sin_addr);
      } else {
        struct sockaddr_in6 *sa = (struct sockaddr_in6 *) ifa->ifa_netmask;
        inet_ntop(AF_INET6, (void *) &sa->sin6_addr, addr, (sizeof(char) * 16));
      }
      break;
    }
  }
  freeifaddrs(ifap);

	if (!addr)
	  addr = o_strdup("255.255.255.0");

	return addr;
}

static int getInterfaceType(char *pchIfName, BOOL isIPv6) {

  char *pchQuery;
  char *pchTable;
  char *pchParamDHCP;
  int interfaceType = 0;
  struct _h_result result;

  if (!o_strcmp(pchIfName, "eth0")) {
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

  pchQuery = msprintf("SELECT GROUP_CONCAT( %s, ',' ) as dhcp FROM %s",pchTable, pchParamDHCP);
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
    pf = popen("/etc/resolvIPv6.conf", "r");  
  } else {
    pf = popen("/etc/resolvIPv4.conf", "r");
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
  char line[200]; 

  if (j_result == NULL) {
    return FALSE;
  }

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return FALSE;
  } 

  pf = popen("/proc/uptime", "r");
  if (pf) {

    char *pchTmpOp = malloc(sizeof(char) * SIZE_STR_TMP_OP);
    memset(pchTmpOp, 0, SIZE_STR_TMP_OP);
    fgets(pchTmpOp, SIZE_STR_TMP_OP, pf);

    json_object_set_new(j_data, "tmp_op", json_string(pchTmpOp));

    pclose(pf);
    o_free(pchTmpOp);
  } 

  pf = popen("uname -n", "r");
  if (pf) {

    char *pchUname = malloc(sizeof(char) * SIZE_STR_UNAME);
    memset(pchUname, 0, SIZE_STR_UNAME);
    fgets(pchUname, SIZE_STR_UNAME, pf);

    json_object_set_new(j_data, "host_name", json_string(pchUname));

    pclose(pf);
    o_free(pchUname);
  } 

  json_object_set_new(j_data, "tmp_ntp", json_string(""));
  json_object_set_new(j_data, "date", json_string(""));
  json_object_set_new(j_data, "time24h", json_string(""));
  json_object_set_new(j_data, "time12h", json_string(""));

  json_object_set_new(j_data, "hwVersion", json_string("1"));
  json_object_set_new(j_data, "swMajor", json_string(systemGeneral.pchswMajor));
  json_object_set_new(j_data, "swMinor", json_string(systemGeneral.swMinor));  
  json_object_set_new(j_data, "swPatch", json_string(systemGeneral.swPatch));  
  json_object_set_new(j_data, "freePartition", json_string("1"));  

  json_array_append_new(*j_result, j_data);

	return TRUE;
}


