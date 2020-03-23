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

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************
 * INCLUDES
 **************************************************************************/

/**************************************************************************
 * DEFINITIONS
 **************************************************************************/

/* Init config for handset */
#define CFG_HANDSETS_NUMBER     "handsetGeneral:handsetNumber"
#define CFG_HANDSETS_VERSION    "handsetGeneral:version"
#define CFG_CHANNELS_NUMBER     "handsetGeneral:channelsNumber"
#define CFG_HANDSET_BUSID       "bus:handset_id"
#define CFG_MULTCAST_SUPPORT    "handsetGeneral:multicastSupport"

#define CFG_ACCOUNTS_NUMBER 	  "controlCall:accountNumber"
#define CFG_PRODUCT		  		    "general:product"
#define CFG_PRODUCT_VERSION     "general:version"
#define CFG_BRANCH		  		    "general:branch"
#define CFG_DATABASE	  		    "general:database_path"
#define CFG_DEV_ID	  		      "general:dev_id"

// Handset Phone
#define CFG_HANDSET_TYPE        "handset%d:type"
#define CFG_NUMBER_SOFTKEYS     "handset%d:numberSoftkeys"
#define CFG_MODULE_SUPPORT      "handset%d:moduleSupport"
#define CFG_NUMBER_MODULES      "handset%d:numberModules"
#define CFG_NUMBER_KEYS_MODULE  "handset%d:numberKeysModule"
#define CFG_DISPLAY_SUPPORT     "handset%d:displaySupport"
#define CFG_CONF_SUPPORT        "handset%d:conferenceMenuKey"
#define CFG_BACKLIGHT_SUPPORT   "handset%d:backlightSupport"
#define CFG_DISPLAY_SIZE        "handset%d:displaySize"

#define PARAM_CONFIG_NAME_SIZE  50

/**************************************************************************
 * FUNÇÕES DE CHAMADA INTERNA
 **************************************************************************/

#ifdef __cplusplus
}
#endif

#endif 
