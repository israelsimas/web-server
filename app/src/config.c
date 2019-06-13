/**************************************************************************
 *
 * config.c
 *
 *    Configuration the plataform
 *
 * Copyright 2017 Intelbras
 *
 **************************************************************************/
#include <config.h>
#include <iniparser.h>
#include <string.h>
#include <misc.h>

#define THIS_FILE "config.c"

dictionary  *pIniFile;

BOOL openConfig() {

  pIniFile = iniparser_load(CONFIG_FILE_PATH);
  if (pIniFile == NULL) {
      LOG_ERROR("Cannot parse file: %s", CONFIG_FILE_PATH);
      return FALSE;
  }

  return TRUE;
}

BOOL getConfig(char *pchParamName, void *pParamValue, E_PARAM_TYPE eType) {

	BOOL statusCfg = TRUE;

	switch(eType) {
		case TYPE_CHAR:
		case TYPE_STRING:
			*((char **)pParamValue) = strdup(iniparser_getstring(pIniFile, pchParamName, NULL));
			break;

		case TYPE_BYTE:
		case TYPE_HEX:
		case TYPE_INT:
		case TYPE_WORD:
		case TYPE_DWORD:
			*((WORD *)pParamValue) = iniparser_getint(pIniFile, pchParamName, -1);
			break;

		case TYPE_DOUBLE:
			*((DWORD *)pParamValue) = iniparser_getdouble(pIniFile, pchParamName, -1);
			break;

		case TYPE_BOOL:
			*((BOOL *)pParamValue) = iniparser_getboolean(pIniFile, pchParamName, FALSE);
			break;

		default:
			statusCfg = FALSE;
			break;
	}

	return statusCfg;
}

void closeConfig() {
  iniparser_freedict(pIniFile);
}
