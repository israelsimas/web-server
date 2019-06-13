/**************************************************************************
 * config.h
 *
 *  Create on: 02/09/2017
 *     Author: Intelbras SIP team
 *
 *  Header File to Plataform SIP Intelbras
 *
 * Copyrights Intelbras, 2017
 *
 **************************************************************************/

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

/**************************************************************************
 * INCLUDES
 **************************************************************************/
#include <defTypes.h>

/**************************************************************************
 * DEFINITIONS
 **************************************************************************/
#define CONFIG_FILE_PATH      "/etc/resources.ini"

#define CONFIG_RELEASE_PATH     "/etc/"
#define CONFIG_RELEASE_FILE     "release"
#define PRODUCT_VERSION       "version "

/* Init config for handset */
#define CFG_HANDSETS_NUMBER     "handsetGeneral:handsetNumber"
#define CFG_HANDSETS_VERSION    "handsetGeneral:version"
#define CFG_CHANNELS_NUMBER     "handsetGeneral:channelsNumber"
#define CFG_HANDSET_BUSID       "bus:handset_id"
#define CFG_MULTCAST_SUPPORT    "handsetGeneral:multicastSupport"

#define CFG_ACCOUNTS_NUMBER 	"controlCall:accountNumber"
#define CFG_PRODUCT		  		"general:product"
#define CFG_PRODUCT_VERSION     "general:version"
#define CFG_BRANCH		  		"general:branch"
#define CFG_DATABASE	  		"general:database_path"

// Handset Phone
#define CFG_HANDSET_TYPE      "handset%d:type"
#define CFG_NUMBER_SOFTKEYS     "handset%d:numberSoftkeys"
#define CFG_MODULE_SUPPORT      "handset%d:moduleSupport"
#define CFG_NUMBER_MODULES      "handset%d:numberModules"
#define CFG_NUMBER_KEYS_MODULE    "handset%d:numberKeysModule"
#define CFG_DISPLAY_SUPPORT     "handset%d:displaySupport"
#define CFG_CONF_SUPPORT      "handset%d:conferenceMenuKey"
#define CFG_BACKLIGHT_SUPPORT   "handset%d:backlightSupport"
#define CFG_DISPLAY_SIZE      "handset%d:displaySize"

#define PARAM_CONFIG_NAME_SIZE    50

/**************************************************************************
 * TYPEDEFS
 **************************************************************************/

/*
 * @enum E_PARAM_TYPE
 *
 * Elements for basic parameters 
 */
typedef enum {
  TYPE_INVALID,
  TYPE_CHAR,
  TYPE_BYTE,
  TYPE_HEX,
  TYPE_INT,
  TYPE_WORD,
  TYPE_DWORD,
  TYPE_BOOL,
  TYPE_STRING,
  TYPE_DOUBLE
} E_PARAM_TYPE;

/**************************************************************************
 * FUNÇÕES DE CHAMADA INTERNA
 **************************************************************************/

/*
 * Realize a open to a file with system configs.
 */
BOOL openConfig();

/**
 * Recupera um ou mais parâmetros do arquivo de Recursos do sistema.
 *
 * @param pParam pointer to parameters that need to be load.
 * @param pchParamName pointer name to parameter.
 * @param pchParamValue pointer to value.
 * @param wLength Lenght buffer that returns.
 *
 * @return BOOL Inidicate that parameter was load with success.
 */
BOOL getConfig(char *pchParamName, void *pParamValue, E_PARAM_TYPE eType);


/**
 * Close the config instance.
 */
void closeConfig();

#endif 
