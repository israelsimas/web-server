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
FILE *pFileFirmware = NULL;
struct _h_connection *connDatabase;

void initFileProcess(struct _h_connection *connDB) {
  connDatabase = connDB;
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
}
