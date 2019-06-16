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

void initFileProcess(struct _h_connection *connDB) {
  connDatabase = connDB;
}

static BOOL isValidFirmware() {
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
      bValidFirmware = isValidFirmware();
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

  char *pchCmd = msprintf("cp %s /data/images/", UPLOAD_FILENAME_PATCH);
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

int updateFirmware() {

  int status = SUCCESS;

  if (bValidFirmware) {
    return SUCCESS;
  } else {
    status = ERROR;
  }

  system("rm /run/firmware.bin");

  return status;
}
