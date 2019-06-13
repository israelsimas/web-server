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

static void loadSystemGeneral(SYSTEM_GENERAL *pSystemGeneral) {

	getConfig(CFG_ACCOUNTS_NUMBER, &pSystemGeneral->accountNumber, TYPE_WORD);
	getConfig(CFG_PRODUCT, &pSystemGeneral->pchProduct, TYPE_STRING);
	getConfig(CFG_PRODUCT_VERSION, &pSystemGeneral->pchVersion, TYPE_STRING);
	getConfig(CFG_BRANCH, &pSystemGeneral->pchBranch, TYPE_STRING);
	getConfig(CFG_DATABASE, &pSystemGeneral->pchDatabasePath, TYPE_STRING);
}

void initSystemGeneral() {
  openConfig();
  loadSystemGeneral(&systemGeneral);
  closeConfig();
}

SYSTEM_GENERAL* getSystemGeneral() {
  return &systemGeneral;
}

char *getUserAccount(WORD wAccount) {

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

BOOL getStatusNetwork(json_t ** j_result) {

	return TRUE;
}

BOOL getStatusSystem(json_t ** j_result) {

	return TRUE;
}


