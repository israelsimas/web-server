/**************************************************************************
 *
 * file-process.c
 *
 *    Get general information to web service
 *
 * Copyright 2019 Intelbras
 *
 **************************************************************************/

#include <base64.h>
#include <database.h>
#include <config.h>
#include <jansson.h>
#include <utils.h>
#include <system-status.h>
#include <system-request.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <file-process.h>

#define THIS_FILE "file-process.c"

FILE *pFileConfig, *pFileLogo, *pFilePatch, *pFileRing, *pFileContacts, *pFileFirmware;
struct _db_connection *connDatabase;
bool bValidFirmware = true;
int statusFirmware  = FIRMWARE_VALID;
char *pchVersionFirmware = NULL;

void initFileProcess(struct _db_connection *connDB) {
  connDatabase  = connDB;
  pFileConfig   = NULL;
  pFileLogo     = NULL;
  pFilePatch    = NULL;
  pFileRing     = NULL;
  pFileContacts = NULL;
  pFileFirmware = NULL;  
}

int getFirmwareStatus() {
  return statusFirmware;
}

static bool isValidFirmware(const char *data) {

  FIRMWARE_HEADER *pFirmHeader;
  SYSTEM_GENERAL *pSystem = getSystemGeneral();
  char *pchData = (char *)data;

  pchData++; // 0x55 jump (header byte)

  pFirmHeader = (FIRMWARE_HEADER *)pchData;
  
  if (data[0] != FIRMWARE_HEADER_BYTE) {
    statusFirmware = FIRMWARE_INVALID_FILE;
    return false;
  }

  if (pSystem->dev_id != pFirmHeader->dev_id) {
    statusFirmware = FIRMWARE_INVALID_PRODUCT;
    return false;
  }

  if ((pSystem->wMajor == pFirmHeader->major) && (pSystem->wMinor == pFirmHeader->minor) && (pSystem->wPatch == pFirmHeader->patch)) {
    statusFirmware = FIRMWARE_INVALID_VERSION;
    return false;
  }

  pchVersionFirmware  = msprintf(VERSION_FIRMWARE_TEMPLATE, pFirmHeader->major, pFirmHeader->minor, pFirmHeader->patch);
  statusFirmware      = FIRMWARE_VALID;
  return true;
}

static char *getFileName(E_UPLOAD_FILE_TYPE eType) {

  switch (eType) {

    case UPLOAD_FILE_CONFIG:
      return UPLOAD_FILENAME_CONFIG;

    case UPLOAD_FILE_LOGO:
      return UPLOAD_FILENAME_LOGO;

    case UPLOAD_FILE_PATCH:
      return UPLOAD_FILENAME_PATCH;

    case UPLOAD_FILE_RING:
      return UPLOAD_FILENAME_RING;

    case UPLOAD_FILE_CONTACTS:
      return UPLOAD_FILENAME_CONTACTS;      

    case UPLOAD_FILE_FIRMWARE:
      return UPLOAD_FILENAME_FIRMWARE;

    default:
      log_error(INVALID_FILE_UPLOAD_MSG);
      return NULL;
  }
}

static void updateFileUpload(E_UPLOAD_FILE_TYPE eType, FILE *pFile) {

  switch (eType) {

    case UPLOAD_FILE_CONFIG:
      pFileConfig = pFile;
      break;

    case UPLOAD_FILE_LOGO:
      pFileLogo = pFile;
      break;

    case UPLOAD_FILE_PATCH:
      pFilePatch = pFile;
      break;

    case UPLOAD_FILE_RING:
      pFileRing = pFile;
      break;

     case UPLOAD_FILE_CONTACTS:
      pFileContacts = pFile;
      break;     

    case UPLOAD_FILE_FIRMWARE:
      pFileFirmware = pFile;
      break;

    default:
      log_error(INVALID_FILE_UPLOAD_MSG);
      return;
      break;
  } 
}

static FILE *getFileUpload(E_UPLOAD_FILE_TYPE eType) {

  switch (eType) {

    case UPLOAD_FILE_CONFIG:
      return pFileConfig;

    case UPLOAD_FILE_LOGO:
      return pFileLogo;

    case UPLOAD_FILE_PATCH:
      return pFilePatch;

    case UPLOAD_FILE_RING:
      return pFileRing;

    case UPLOAD_FILE_CONTACTS:
      return pFileContacts;      

    case UPLOAD_FILE_FIRMWARE:
      return pFileFirmware;

    default:
      log_error(INVALID_FILE_UPLOAD_MSG);
      return NULL;
  }  
}

void loadUploadFile(const char *data, uint64_t off, size_t size, E_UPLOAD_FILE_TYPE eType) {

  FILE *pFile = NULL;
  
  if (IS_FIRST_PACKET(off)) {

    char *pchRemoveCmd, *pchFileName;

    pchFileName   = getFileName(eType);
    pchRemoveCmd  = msprintf("rm %s", pchFileName);
    system(pchRemoveCmd);

    pFile = fopen(pchFileName, "w");
    if (pFile) {
      fwrite(data, size, 1, pFile);
    }
    updateFileUpload(eType, pFile);

    if (eType == UPLOAD_FILE_FIRMWARE) {
      bValidFirmware = isValidFirmware(data);
      if (bValidFirmware) {
        generalNotify(FIRMWARE_UPDATE_DONE);
        stopAppsSystem();
      } else {
        generalNotify(FIRMWARE_UPDATE_DONE);
      }
    }

  } else if ((eType == UPLOAD_FILE_FIRMWARE) && !bValidFirmware) {
    // do nothing - Invalid Firmware
  } else {
    pFile = getFileUpload(eType);
    if (pFile) {
      fwrite(data, size, 1, pFile);
    }
  }
}

void closeUploadFile(E_UPLOAD_FILE_TYPE eType) {

  switch (eType) {

    case UPLOAD_FILE_CONFIG:
      fclose(pFileConfig);
      pFileConfig = NULL;
      break;

    case UPLOAD_FILE_LOGO:
      fclose(pFileLogo);
      pFileLogo = NULL;
      break;

    case UPLOAD_FILE_PATCH:
      fclose(pFilePatch);
      pFilePatch = NULL;
      break;

    case UPLOAD_FILE_RING:
      fclose(pFileRing);
      pFileRing = NULL;
      break;

     case UPLOAD_FILE_CONTACTS:
      fclose(pFileContacts);
      pFileContacts = NULL;
      break;     

    case UPLOAD_FILE_FIRMWARE:
      fclose(pFileFirmware);
      pFileFirmware = NULL;
      break;

    default:
      log_error(INVALID_FILE_UPLOAD_MSG);
      break;
  }
}

void updateConfig() {
  char *pchMac = ntw_get_mac(DEFAULT_INTERFACE, false);
  char *pchQuery;

  if (pchMac) {
    system("rm /data/database.sql && sync");
    system("openssl enc -aes-256-cbc -d -in /tmp/config.db -out /data/database.sql -k SIRIUS_INTELBRAS");
    pchQuery = msprintf("UPDATE TAB_NET_ETH_WAN SET ETHMAC = '%s'", pchMac);
    db_query_update(connDatabase, pchQuery);
    o_free(pchQuery);

    system("sync && reboot &");
  }

  system("rm /tmp/config.db");
}

static bool addRing(const char *pchRingName) {

  char *pchQuery = msprintf("INSERT INTO TAB_SYSTEM_RING (Account, SYSRingtype, SYSRingName) VALUES (0, 2, '%s');", pchRingName);

  if (!pchQuery) {
    return false;
  }

  if (db_query_insert(connDatabase, pchQuery) == DATABASE_OK) {
    return true;
  } else {
    return false;
  }
} 

static int getPkFromLastInsertedRing() {

  struct _db_result result;

  if (db_query_select(connDatabase, "SELECT MAX(PK) as PK FROM TAB_SYSTEM_RING", &result) == DATABASE_OK) {
    if (result.nb_rows == 1) {

      int pk = ((struct _db_type_int *)result.data[0][0].t_data)->value;
      return pk;
    }
  }

  return INVALID_RING_PK;
}

void updateRing(const char *pchRingName) {

	if (addRing(pchRingName)) {
    int ringId = getPkFromLastInsertedRing();
    char *pchCommand;

    if (ringId != INVALID_RING_PK) {
      pchCommand = msprintf("wav-pcmu %s /data/rings/%d.wav", UPLOAD_FILENAME_RING, ringId);
      system(pchCommand);
      o_free(pchCommand);     
    }

  }

  system("rm /tmp/ring.wav");
}

void updateLogo() {

  char *pchCmd = msprintf("cp %s /data/images/", UPLOAD_FILENAME_LOGO);

  system(pchCmd);
  o_free(pchCmd);

  system("rm /tmp/logo.bmp");
}

void updatePatch() {

  char *pchCmd = NULL;
  SYSTEM_GENERAL *pSystem = getSystemGeneral();

  pchCmd = msprintf("rm -rf /data/patch ; mkdir /data/patch ; openssl enc -k SIRIUS_INTELBRAS -d -aes256 -in %s | tar x -C /data/patch", UPLOAD_FILENAME_PATCH);
  system(pchCmd);
  o_free(pchCmd);

  pchCmd = msprintf("lua /data/patch/install.lua /data/patch %s %s", pSystem->pchProduct, pSystem->pchVersion);
  system(pchCmd);
  o_free(pchCmd);

  system("rm -rf /data/patch");
  system("rm /data/patch_new.patch");
}

void updateContacts() {

  system("rm /tmp/contacts.xml");
}

static char *getFreePartition() {

  FILE *pf;
  char *pchPartion = PARTITION_1;
  char pchCmdRet[SIZE_STR_PARTITION];

   pf = popen("fw_printenv rootfspart | cut -d\"=\" -f2'", "r");
   if (pf) {

    memset(pchCmdRet, 0, SIZE_STR_PARTITION);
    fgets(pchCmdRet, SIZE_STR_PARTITION, pf);

    if (o_strcmp(pchCmdRet, "rootfs") == 0) {
      pchPartion = PARTITION_2;
    } else {
      pchPartion = PARTITION_1;
    }
    pclose(pf);
  }  

  return pchPartion;
}

int updateFirmware() {

  int status    = SUCCESS;
  char *pchCmd  = NULL;

#if 0 // TODO Firmware burner 
  pchCmd = msprintf("burn-firmware  -f %s -p %s", UPLOAD_FILENAME_FIRMWARE, getFreePartition());
  system(pchCmd);
  o_free(pchCmd);
#endif  

  return status;
}

void closeFwupdate(char **ppchMessage) {

  SYSTEM_GENERAL *pSystem = getSystemGeneral();

  system("rm /run/firmware.bin");

  *ppchMessage = msprintf("Alterando da v%d.%d.%d para v%s <br/>Stored on partition %d <br/>Restarting the device!", pSystem->wMajor, pSystem->wMinor, pSystem->wPatch, pchVersionFirmware);

  system("/etc/rc5.d/S95control-call unclok");
  system("sync && reboot &");
  generalNotify("fw_update_done");
}

void stopAppsSystem() {
  system("/etc/rc5.d/S95control-call lock && killall control-call");
  system("/etc/rc5.d/S91dspg_apps stop");
  system("/etc/rc5.d/S93handset lock && killall handset");
  system("/etc/rc5.d/S87middleware stop");
  system("echo \"-1\" > /tmp/burningPercent");
}

void restartAppsSystem() {
  system("/etc/rc5.d/S87middleware restart");
  system("/etc/rc5.d/S91dspg_apps start");
  system("/etc/rc5.d/S93handset unlock");
  system("/etc/rc5.d/S95control-call unlock");
}

int percentageNum = 0;

bool getBurningStatus(json_t *j_result) {
  
  FILE *pFile = NULL;
  char *pchPercent;
  int length;

  if (j_result == NULL) {
    return false;
  }

  pFile = fopen(FIRMWARE_UPLOAD_STATUS_FILE, "r");
  if (pFile) {
    fseek(pFile, 0, SEEK_END);
    length = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);
    pchPercent = o_malloc(length*sizeof(void));
    if (pchPercent) {
      fread(pchPercent, 1, length, pFile);
    }
    fclose(pFile);
  } else {
      pchPercent = o_malloc(PERCENTAGE_STR_LEN*sizeof(void));
      sprintf(pchPercent, "%d", 0);
  }

  json_object_set_new(j_result, "percent", json_string(pchPercent));

	return true; 
}
