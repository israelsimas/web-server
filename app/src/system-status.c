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

BOOL getStatusNetwork(json_t **j_result) {

  json_t *j_data;
  char *pchInterface;
  int protocolMode;

  if (j_result == NULL) {
    return FALSE;
  }

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return FALSE;
  } 

  pchInterface = getActiveInterface();
  protocolMode = getProtocolMode();

  if ((protocolMode == 0) || (protocolMode == 2)) {
    char *pchIPv4 = getIfaddr(pchInterface, AF_INET);

    json_object_set_new(j_data, "add_ipv4", json_string(pchIPv4));
    json_object_set_new(j_data, "netmask", json_string("255.255.255.0"));
    json_object_set_new(j_data, "gateway_ipv4", json_string("10.1.39.1"));
    json_object_set_new(j_data, "type_ipv4", json_string("0"));
    json_object_set_new(j_data, "dns1_ipv4", json_string("8.8.8.8"));
    json_object_set_new(j_data, "dns2_ipv4", json_string(""));

    o_free(pchIPv4);
  } else {
    json_object_set_new(j_data, "add_ipv4", json_string(""));
    json_object_set_new(j_data, "netmask", json_string(""));
    json_object_set_new(j_data, "gateway_ipv4", json_string(""));
    json_object_set_new(j_data, "type_ipv4", json_string(""));
    json_object_set_new(j_data, "dns1_ipv4", json_string(""));
    json_object_set_new(j_data, "dns2_ipv4", json_string(""));
  }

  if ((protocolMode == 1) || (protocolMode == 2)) {
    char *pchIPv6 = getIfaddr(pchInterface, AF_INET6);

    json_object_set_new(j_data, "add_ipv6", json_string(pchIPv6));
    json_object_set_new(j_data, "gateway_ipv6", json_string("0::0"));
    json_object_set_new(j_data, "type_ipv6", json_string("0"));
    json_object_set_new(j_data, "dns1_ipv6", json_string("0::0"));
    json_object_set_new(j_data, "dns2_ipv6", json_string("0::0"));

    o_free(pchIPv6);
  } else {
    json_object_set_new(j_data, "add_ipv6", json_string(""));
    json_object_set_new(j_data, "gateway_ipv6", json_string(""));
    json_object_set_new(j_data, "type_ipv6", json_string(""));
    json_object_set_new(j_data, "dns1_ipv6", json_string(""));
    json_object_set_new(j_data, "dns2_ipv6", json_string(""));
  }  

  json_object_set_new(j_data, "mac", json_string("00:1A:3F:01:02:03"));

  json_object_set_new(j_data, "prot_mode", json_integer(protocolMode));

  json_array_append_new(*j_result, j_data);

  o_free(pchInterface);

	return TRUE;
}

BOOL getStatusSystem(json_t **j_result) {

  json_t *j_data;
  char *pchInterface;
  int protocolMode;

  if (j_result == NULL) {
    return FALSE;
  }

  j_data = json_object();
  if (j_data == NULL) {
    json_decref(*j_result); 
    return FALSE;
  } 

  json_object_set_new(j_data, "tmp_op", json_string("12131"));
  json_object_set_new(j_data, "tmp_ntp", json_string(""));
  json_object_set_new(j_data, "date", json_string(""));
  json_object_set_new(j_data, "time24h", json_string(""));
  json_object_set_new(j_data, "time12h", json_string(""));
  json_object_set_new(j_data, "hwVersion", json_string("1"));
  json_object_set_new(j_data, "swMajor", json_string("1"));
  json_object_set_new(j_data, "swMinor", json_string("2"));  
  json_object_set_new(j_data, "swPatch", json_string("3"));  
  json_object_set_new(j_data, "freePartition", json_string("1"));  

  json_array_append_new(*j_result, j_data);

	return TRUE;
}


