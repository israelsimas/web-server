
/**************************************************************************
 * system-request.h
 *
 *  Create on: 07/06/2019
 *     Author: Intelbras SIP team
 *
 *  Header File to Plataform Sirius Intelbras
 *
 * Copyrights Intelbras, 2019
 *
 **************************************************************************/

#ifndef SYTEM_REQUEST_H_
#define SYTEM_REQUEST_H_

#include <ulfius.h>

/**************************************************************************
 * DEFINITIONS
 **************************************************************************/

#define PARTITION_1 "rootfs"
#define PARTITION_2 "rootfs-2"

/**************************************************************************
 * TYPEDEFS
 **************************************************************************/

#define PREFIX_PATH_LOGS "/data"

/**************************************************************************
 * INTERNAL FUNCTIONS
 **************************************************************************/

void restartSystem();

void restartSyslog();

void factoryReset();

void logoReset();

void setLanguage();

void notifyTables(const char *pchTables);

void generalNotify(const char *pchTables);

void setNTPDateTime();

void setManualDateTime(const char *pchDate);

void setChangeBootPartition(int bootPartition);

void setSelfProvision(struct _u_map *map_url);

void startCaptureLog();

void stopCaptureLog();

int getAutoprovXML(char **ppchBuffer);

int getContactXML(char **ppchBuffer);

#endif
