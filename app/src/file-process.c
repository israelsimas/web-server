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
#include <misc.h>
#include <hoel.h>
#include <config.h>
#include <jansson.h>
#include <misc.h>
#include <system-status.h>
#include <system-request.h>
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
#include <file-process.h>

#define THIS_FILE "file-process.c"

FILE *pFileConfig = NULL;
FILE *pFileLogo = NULL;
FILE *pFilePatch = NULL;
FILE *pFileRing = NULL;
FILE *pFileContacts = NULL;
FILE *pFileFirmware = NULL;
struct _h_connection *connDatabase;
BOOL bValidFirmware = TRUE;
int statusFirmware = FIRMWARE_VALID;
char *pchVersionFirmware = NULL;

void initFileProcess(struct _h_connection *connDB) {
  connDatabase = connDB;
}

int getFirmwareStatus() {
  return statusFirmware;
}

static BOOL isValidFirmware(const char *data) {
  FIRMWARE_HEADER *pFirmHeader;
  SYSTEM_GENERAL *pSystem = getSystemGeneral();
  char *pchData = data;

  pchData++;
  pFirmHeader = (FIRMWARE_HEADER *)pchData;
  
  if (data[0] != FIRMWARE_HEADER_BYTE) {
    statusFirmware = FIRMWARE_INVALID_FILE;
    return FALSE;
  }

  if (pSystem->dev_id != pFirmHeader->dev_id) {
    statusFirmware = FIRMWARE_INVALID_PRODUCT;
    return FALSE;
  }

  if ((pSystem->wMajor == pFirmHeader->major) && (pSystem->wMinor == pFirmHeader->minor) && (pSystem->wPatch == pFirmHeader->patch) ) {
    statusFirmware = FIRMWARE_INVALID_VERSION;
    return FALSE;
  }

  pchVersionFirmware = msprintf("%d.%d.%d", pFirmHeader->major, pFirmHeader->minor, pFirmHeader->patch);
  statusFirmware = FIRMWARE_VALID;
  return TRUE;
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
      LOG_ERROR("Invalid file type to upload");
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
      LOG_ERROR("Invalid file type to upload");
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
      LOG_ERROR("Invalid file type to upload");
      return NULL;
  }  
}

void loadUploadFile(const char *data, uint64_t off, size_t size, E_UPLOAD_FILE_TYPE eType) {

  FILE *pFile = NULL;
  
  if (IS_FIRST_PACKET(off)) {

    char *pchRemoveCmd, *pchFileName;

    pchFileName = getFileName(eType);
    pchRemoveCmd = msprintf("rm %s", pchFileName);
    system(pchRemoveCmd);

    pFile = fopen(pchFileName, "w");
    if (pFile) {
      fwrite(data, size, 1, pFile);
    }
    updateFileUpload(eType, pFile);

    if (eType == UPLOAD_FILE_FIRMWARE) {
      bValidFirmware = isValidFirmware(data);
      if (bValidFirmware) {
        generalNotify("fw_update_done");
        stopAppsSystem();
      } else {
        generalNotify("fw_update_done");
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
      LOG_ERROR("Invalid file type to upload");
      break;
  }
}

void updateConfig() {
  char *pchMac = getMac();
  char *pchQuery;

  if (pchMac) {
    system("rm /data/database.sql && sync");
    system("openssl enc -aes-256-cbc -d -in /tmp/config.db -out /data/database.sql -k SIRIUS_INTELBRAS");
    pchQuery = msprintf("UPDATE TAB_NET_ETH_WAN SET ETHMAC = '%s'", pchMac);
    h_query_update(connDatabase, pchQuery);
    o_free(pchQuery);

    system("sync && reboot &");
  }

  system("rm /tmp/config.db");
}

static BOOL addRing(char *pchRingName) {

  char *pchQuery = msprintf("INSERT INTO TAB_SYSTEM_RING (Account, SYSRingtype, SYSRingName) VALUES (0, 2, '%s');", pchRingName);

  if (!pchQuery) {
    return FALSE;
  }

  if (h_query_insert(connDatabase, pchQuery) == H_OK) {
    return TRUE;
  } else {
    return FALSE;
  }
} 

static int getPkFromLastInsertedRing() {

  struct _h_result result;

  if (h_query_select(connDatabase, "SELECT MAX(PK) as PK FROM TAB_SYSTEM_RING", &result) == H_OK) {
    if (result.nb_rows == 1) {

      int pk = ((struct _h_type_int *)result.data[0][0].t_data)->value;
      return pk;
    }
  }

  return INVALID_RING_PK;
}

void updateRing(char *pchRingName) {

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

  int status = SUCCESS;
  char *pchCmd = NULL;

  // pchCmd = msprintf("burn-firmware  -f %s -p %s", UPLOAD_FILENAME_FIRMWARE, getFreePartition());
  // system(pchCmd);
  // o_free(pchCmd);

//  sleep(60);

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
  system("/etc/rc5.d/S87middleware-zeromq stop");
  system("echo \"-1\" > /tmp/burningPercent");
}

void restartAppsSystem() {
  system("/etc/rc5.d/S87middleware-zeromq restart");
  system("/etc/rc5.d/S91dspg_apps start");
  system("/etc/rc5.d/S93handset unlock");
  system("/etc/rc5.d/S95control-call unlock");
}

int percentageNum = 0;

BOOL getBurningStatus(json_t *j_result) {
  
  FILE *pFile = NULL;
  char *pchPercent;
  int length;

  if (j_result == NULL) {
    return FALSE;
  }

  pFile = fopen (FIRMWARE_UPLOAD_STATUS_FILE, "r");
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

	return TRUE; 
}
