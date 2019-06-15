/**************************************************************************
 * system-status.h
 *
 *  Create on: 07/06/2019
 *     Author: Intelbras SIP team
 *
 *  Header File to Plataform Sirius Intelbras
 *
 * Copyrights Intelbras, 2019
 *
 **************************************************************************/

#ifndef SYTEM_STATUS_H_
#define SYTEM_STATUS_H_

#if 1
#define DEFAULT_INTERFACE "eth0"
#else
#define DEFAULT_INTERFACE "wlp3s0"
#endif

#define INVALID_IP        "0"
#define SIZE_STR_GATEWAY  40
#define SIZE_STR_MAC      40
#define SIZE_STR_STATUS_SYS      40
#define BUFFER_REG_LENGHT 		   15
#define PORT_SERVER_REG_STATUS   4849
#define ENDPOINT_BUSY     0

#define BUFFER_ENDP_LENGHT 	20
#define ENDPOINT_STATUS 		"CMD_STATUS"
#define SOCK_STATUS_ADDR		"/tmp/endpointstatus"

#define PERCENTAGE_STR_LEN  4

/**************************************************************************
 * TYPEDEFS
 **************************************************************************/
typedef struct SYSTEM_GENERAL {
  char *pchProduct;
  char *pchBranch;
  char *pchDatabasePath;
  char *pchVersion;
  char *pchswMajor;
  char *swMinor;
  char *swPatch;   
  int accountNumber;
} SYSTEM_GENERAL;

/**************************************************************************
 * INTERNAL FUNCTIONS
 **************************************************************************/

void initSystemGeneral(struct _h_connection *pConn);

SYSTEM_GENERAL *getSystemGeneral();

BOOL getRegisterStatusAccount(json_t ** j_result, WORD wAccount);

BOOL getEndpointFreeStatus(json_t ** j_result);

BOOL getStatusAccount(json_t ** j_result);

BOOL getStatusNetwork(json_t ** j_result);

BOOL getStatusSystem(json_t ** j_result);

BOOL getGeneralStatus(json_t **j_result);

BOOL getGigaSupport(json_t **j_result);

BOOL getVersionStatus(json_t **j_result);

BOOL getBurningStatus(json_t **j_result);

#endif
