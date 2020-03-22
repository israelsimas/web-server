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

#define FIRMWARE_UPLOAD_STATUS_FILE "/tmp/burningPercent"

#define IS_FIRST_PACKET(offset) ((offset==0)?true:false)

#define INVALID_RING_PK 0

#define FIRMWARE_HEADER_BYTE  0x55

#define SIZE_STR_PARTITION  10

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

typedef struct FIRMWARE_HEADER {
	unsigned int vendor;
	unsigned int dev_id;
	unsigned short major;
	unsigned short minor;
	unsigned short patch;
} FIRMWARE_HEADER;

typedef enum {
	FIRMWARE_VALID			        = 0,
  FIRMWARE_INVALID_FILE			  = -1,
	FIRMWARE_INVALID_PRODUCT	  = -2,
	FIRMWARE_INVALID_VERSION	  = -3,
} E_FIRMWARE_STATUS;


/**************************************************************************
 * INTERNAL FUNCTIONS
 **************************************************************************/

void initFileProcess(struct _h_connection *connDB);

void loadUploadFile(const char *data, uint64_t off, size_t size, E_UPLOAD_FILE_TYPE eType);

void closeUploadFile(E_UPLOAD_FILE_TYPE eType);

void updateConfig();

void updateRing(const char *pchRingName);

void updateLogo();

void updatePatch();

void updateContacts();

int getFirmwareStatus();

int updateFirmware();

void stopAppsSystem();

void restartAppsSystem();

void closeFwupdate(char **ppchMessage);

bool getBurningStatus(json_t *j_result);

#endif
