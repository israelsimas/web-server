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

BOOL getStatusNetwork(json_t ** j_result) {

  char *pchInterface = getActiveInterface();

	return TRUE;
}

BOOL getStatusSystem(json_t ** j_result) {

	return TRUE;
}


