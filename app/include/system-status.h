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

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************
 * INCLUDES
 **************************************************************************/
#include <network.h>

/**************************************************************************
 * DEFINITIONS
 **************************************************************************/

#ifdef __APPLE__
  #define DEFAULT_INTERFACE "en0"
#elif __linux__
  #if 1
  #define DEFAULT_INTERFACE "eth0"
  #else
  #define DEFAULT_INTERFACE "wlp3s0"
  #endif
#endif

#define SIZE_STR_GATEWAY          40
#define SIZE_STR_MAC              40
#define SIZE_STR_STATUS_SYS       40
#define BUFFER_REG_LENGHT 		    15
#define PORT_SERVER_REG_STATUS    4849
#define ENDPOINT_BUSY             0
#define VERSION_LENGHT            10
#define ACCOUNT_LENGHT            3
#define USER_FIELD_LENGHT         10
#define PRODUCT_NAME_LENGHT       10
#define MODEL_NAME_LENGHT         10

#define BUFFER_ENDP_LENGHT 	      20
#define ENDPOINT_STATUS 		      "CMD_STATUS"
#define PORT_SERVER_ENDPOINT_STATUS 4860

#define PERCENTAGE_STR_LEN        5

#define LOGIN_TIMEOUT             300

#define AUTHENTICATE_REQUEST(req, resp) \
                              do { \
                                if (!isAuthenticated(req, resp)) { \
                                    return U_CALLBACK_UNAUTHORIZED; \
                                } \
                              } while(0)

/**************************************************************************
 * TYPEDEFS
 **************************************************************************/
typedef struct SYSTEM_GENERAL {
  char *pchProduct;
  char *pchBranch;
  char *pchDatabasePath;
  char *pchVersion;
  word wMajor;
  word wMinor;
  word wPatch;   
  int accountNumber;
  int dev_id;
  time_t loginTime;
  char *pchAdminUser;
  char *pchAdminPwd;  
} SYSTEM_GENERAL;

/**************************************************************************
 * INTERNAL FUNCTIONS
 **************************************************************************/

void initSystemGeneral(struct _db_connection *pConn);

SYSTEM_GENERAL *getSystemGeneral();

bool getRegisterStatusAccount(json_t ** j_result, word wAccount);

bool getEndpointFreeStatus(json_t ** j_result);

bool getStatusAccount(json_t ** j_result);

bool getStatusNetwork(json_t ** j_result);

bool getStatusSystem(json_t ** j_result);

bool getGeneralStatus(json_t **j_result);

bool getGigaSupport(json_t **j_result);

bool getVersionStatus(json_t **j_result);

bool getFwCloudVersion(json_t **j_result);

#ifdef __cplusplus
}
#endif

#endif
