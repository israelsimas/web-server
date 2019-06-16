/**************************************************************************
 * file-process.h
 *
 *  Create on: 07/06/2019
 *     Author: Intelbras SIP team
 *
 *  Header File to Plataform Sirius Intelbras
 *
 * Copyrights Intelbras, 2019
 *
 **************************************************************************/

#ifndef FILE_PROCESS_H_
#define FILE_PROCESS_H_

/**************************************************************************
 * DEFINITIONS
 **************************************************************************/

#define UPLOAD_FILENAME_CONFIG    "/tmp/config.db"
#define UPLOAD_FILENAME_LOGO      "/tmp/logo.bmp"
#define UPLOAD_FILENAME_PATCH     "/data/patch_new.patch"
#define UPLOAD_FILENAME_RING      "/tmp/ring.wav"
#define UPLOAD_FILENAME_CONTACTS  "/tmp/contacts.xml"
#define UPLOAD_FILENAME_FIRMWARE  "/run/firmware.bin"

#define IS_FIRST_PACKET(offset) ((offset==0)?TRUE:FALSE)

#define INVALID_RING_PK 0

/**************************************************************************
 * TYPEDEFS
 **************************************************************************/

/**
 * 	@enum E_UPLOAD_FILE_TYPE
 */
typedef enum {
  UPLOAD_FILE_CONFIG,
  UPLOAD_FILE_LOGO,
  UPLOAD_FILE_PATCH,
  UPLOAD_FILE_RING,
  UPLOAD_FILE_CONTACTS,
  UPLOAD_FILE_FIRMWARE
} E_UPLOAD_FILE_TYPE;


/**************************************************************************
 * INTERNAL FUNCTIONS
 **************************************************************************/

void initFileProcess(struct _h_connection *connDB);

void loadUploadFile(const char *data, uint64_t off, size_t size, E_UPLOAD_FILE_TYPE eType);

void closeUploadFile(E_UPLOAD_FILE_TYPE eType);

void updateConfig();

void updateRing(char *pchRingName);

void updateLogo();

void updatePatch();

void updateContacts();

int updateFirmware();

#endif
